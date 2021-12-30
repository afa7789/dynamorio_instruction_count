/* **********************************************************
 * Copyright (c) 2014-2018 Google, Inc.  All rights reserved.
 * Copyright (c) 2008 VMware, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* Instruction Counter:
 * icounter.c
 *
 * Reports the dynamic execution count of all instructions.
 */

#include <stddef.h> /* for offsetof */
#include <stdlib.h> /* for strtoull */
#include <string.h> /* for strtok */
#include "dr_api.h"
#include "drmgr.h"
#include "drreg.h"
#include "drx.h"
#include "hashtable.h"
#include "drvector.h"
#include "utils.h"

// Groups mode:
//   1: list using drvector.
//   2: linked list.
#define GROUPS_MODE 2

/* ===================================================================== */
/* Structs                                                               */
/* ===================================================================== */

// Struct to represent Unique Instruction
typedef struct _UniqueInstr UniqueInstr;
struct _UniqueInstr {
    app_pc addr;
    int size;
    size_t exec_count; // The number of times this instruction was executed.
};

// Representation of a group of unique instructions
typedef struct _InstrGroup InstrGroup;
struct _InstrGroup {
	size_t exec_count;  // The number of times this group was executed.
	struct {
        UniqueInstr** list;
        size_t size;
    } instrs; // The list of instructions of this group.

#if GROUPS_MODE != 1
    InstrGroup* next;
#endif
};


/* ===================================================================== */
/* Globals variables                                                     */
/* ===================================================================== */
FILE* out_file;

static hashtable_t instrs_hash;
#define HASH_BITS 13

#if GROUPS_MODE == 1
static drvector_t groups_list;
#define INITIAL_GROUPS_SIZE 10000
#else
static InstrGroup* groups_list;
static void *groups_mutex;
#endif

/* ===================================================================== */
/*  Form of fetching the instruction or creating a new one               */
/* ===================================================================== */

static UniqueInstr*
fetch_instr(app_pc addr, int size) {
    UniqueInstr* instr;

    hashtable_lock(&instrs_hash);
    instr = hashtable_lookup(&instrs_hash, addr);
    if (instr == 0) {
        instr = (UniqueInstr*) dr_global_alloc(sizeof(UniqueInstr));
        DR_ASSERT(instr != 0);

        instr->addr = addr;
        instr->size = size;
        instr->exec_count = 0;

        if (!hashtable_add(&instrs_hash, addr, (void *) instr))
            DR_ASSERT(false);
    } else {
        DR_ASSERT(instr->size == size);
    }
    hashtable_unlock(&instrs_hash);

    return instr;
}

/* ===================================================================== */
/* Fprintf the instruction received.                                     */
/* ===================================================================== */

static void
dump_instr(UniqueInstr *instr) {
    fprintf(out_file, "%p:%d:%lu\n", instr->addr, instr->size, instr->exec_count);
}

/* ===================================================================== */
/* Release memory                                                        */
/* ===================================================================== */

static void
free_instr(UniqueInstr *instr) {
    dr_global_free(instr, sizeof(UniqueInstr));
}

/* ===================================================================== */
/* Read Instructions of another filename                                 */
/* ===================================================================== */

static void
read_instrs(const char* filename) {
    FILE* f;
    char buffer[255];
    static char sep[] = ":\r\n";

    f = fopen(filename, "r");
    if (f != 0) {
        while (fgets(buffer, sizeof(buffer), f) > 0) {
            app_pc addr;
            int size;
            UniqueInstr* instr;

            addr = (app_pc) strtoull(strtok(buffer+2, sep), 0, 16);
            size = atoi(strtok(0, sep));

            instr = fetch_instr(addr, size);
            instr->exec_count += strtoull(strtok(0, sep), 0, 10);
        }

        fclose(f);
    }
}

/* ===================================================================== */
/* New Instruction Group constructor                                     */
/* ===================================================================== */

static InstrGroup*
new_group(size_t size) {
    InstrGroup* group;

    DR_ASSERT(size > 0);

    group = (InstrGroup*) dr_global_alloc(sizeof(InstrGroup));
    DR_ASSERT(group != 0);

    group->exec_count = 0;
    group->instrs.size = size;
    group->instrs.list = (UniqueInstr**)
        dr_global_alloc(size * sizeof(UniqueInstr*));
    DR_ASSERT(group->instrs.list != 0);

#if GROUPS_MODE == 1
    drvector_lock(&groups_list);
    drvector_append(&groups_list, group);
    drvector_unlock(&groups_list);
#else
    dr_mutex_lock(groups_mutex);
    group->next = groups_list;
    groups_list = group;
    dr_mutex_unlock(groups_mutex);
#endif

    return group;
}

static void
flush_groups() {
    InstrGroup* group;

#if GROUPS_MODE == 1
    size_t index = 0;

    drvector_lock(&groups_list);
    while ((group = drvector_get_entry(&groups_list, index++))) {
        size_t index2 = 0;
        for (index2 = 0; index2 < group->instrs.size; ++index2)
            group->instrs.list[index2]->exec_count += group->exec_count;
    }
    drvector_unlock(&groups_list);
#else
    dr_mutex_lock(groups_mutex);
    group = groups_list;
    while (group != 0) {
        size_t index2 = 0;
        for (index2 = 0; index2 < group->instrs.size; ++index2)
            group->instrs.list[index2]->exec_count += group->exec_count;

        group = group->next;
    }
    dr_mutex_unlock(groups_mutex);
#endif
}

/* ===================================================================== */
/* Free group memory                                                     */
/* ===================================================================== */
static void
free_group(InstrGroup* group) {
    dr_global_free(group->instrs.list,
        (group->instrs.size * sizeof(UniqueInstr*)));
    dr_global_free(group, sizeof(InstrGroup));
}

/* ===================================================================== */
/* Free group list                                                     */
/* ===================================================================== */
#if GROUPS_MODE != 1
static
void free_groups_list() {
    InstrGroup* group;

    dr_mutex_lock(groups_mutex);
    group = groups_list;
    while (group != 0) {
        InstrGroup* next = group->next;
        free_group(group);
        group = next;
    }
    dr_mutex_unlock(groups_mutex);
}
#endif

/* ===================================================================== */
/* Instrumentation by the app event that calls every bb                  */
/* ===================================================================== */

static dr_emit_flags_t
event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *inst,
                      bool for_trace, bool translating, void *user_data) {
    // By default drmgr enables auto-predication, which predicates all
    // instructions with the predicate of the current instruction on ARM.
    // We disable it here because we want to unconditionally execute the
    // following instrumentation.
    drmgr_disable_auto_predication(drcontext, bb);
    if (drmgr_is_first_instr(drcontext, inst)) {
        instr_t *ins;
        size_t index;
        InstrGroup* group;

        index = 0;
        group = new_group(drx_instrlist_app_size(bb));

        // Add the instructions to the group.
        for (ins = instrlist_first_app(bb); ins != 0;
                ins = instr_get_next_app(ins)) {
            app_pc addr;
            int size;

            addr = instr_get_app_pc(ins);
            size = instr_length(drcontext, ins);

            group->instrs.list[index++] = fetch_instr(addr, size);
        }
        DR_ASSERT(index == group->instrs.size);

        // Racy update on the counter for better performance.
        drx_insert_counter_update(drcontext, bb, inst,
                                /* We're using drmgr, so these slots
                                * here won't be used: drreg's slots will be.
                                */
                                SPILL_SLOT_MAX + 1,
                                IF_AARCHXX_(SPILL_SLOT_MAX + 1) &(group->exec_count), 1, 0);

#ifdef VERBOSE
        dr_printf("in dynamorio_basic_block(tag=" PFX ")\n", tag);
#ifdef VERBOSE_VERBOSE
        instrlist_disassemble(drcontext, tag, bb, STDOUT);
#endif
#endif
    }

    return DR_EMIT_DEFAULT;
}

/* ===================================================================== */
/* Event Exit function                                                   */
/* ===================================================================== */

static void
event_exit(void) {
    if (!drmgr_unregister_bb_insertion_event(event_app_instruction))
        DR_ASSERT(false);

    flush_groups();

    if (out_file != 0) {
        hashtable_apply_to_all_payloads(&instrs_hash, (void (*)(void*)) dump_instr);
        fclose(out_file);
    }

#if GROUPS_MODE == 1
    drvector_delete(&groups_list);
#else
    free_groups_list();
    dr_mutex_destroy(groups_mutex);
#endif

    hashtable_delete(&instrs_hash);

    drx_exit();
    drmgr_exit();
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */


DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[]) {
    dr_set_client_name("DynamoRIO Instruction Counter Client",
                       "http://dynamorio.org/issues");

    if (!drmgr_init() || !drx_init())
        DR_ASSERT(false);

    hashtable_init_ex(&instrs_hash, HASH_BITS, HASH_INTPTR, false /*!strdup*/,
                      false /*synchronization is external*/,
                      (void (*)(void*)) free_instr, NULL, NULL);

#if GROUPS_MODE == 1
    drvector_init(&groups_list, INITIAL_GROUPS_SIZE, false /*bool synch*/,
        (void (*)(void *)) free_group);
#else
    groups_list = 0;
    groups_mutex = dr_mutex_create();
#endif

    if (argc > 3) {
        dr_printf("too many arguments");
        dr_abort();
    }

    if (argc > 2)
        read_instrs(argv[2]);

    if (argc > 1) {
        out_file = fopen(argv[1], "w");
        DR_ASSERT(out_file != 0);
    } else {
        out_file = 0;
    }

    // Register events.
    dr_register_exit_event(event_exit);
    if (!drmgr_register_bb_instrumentation_event(NULL, event_app_instruction, NULL))
        DR_ASSERT(false);
}
