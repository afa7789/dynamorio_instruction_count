#include <stddef.h> /* for offsetof */
#include "dr_api.h"
#include "drmgr.h"
#include "drreg.h"
#include "drx.h"
#include "utils.h"
#include "hashmap/hash.h"

#define DISPLAY_STRING(msg) dr_printf("%s\n", msg);

#define NULL_TERMINATE(buf) (buf)[(sizeof((buf)) / sizeof((buf)[0])) - 1] = '\0'

#define TESTALL(mask, var) (((mask) & (var)) == (mask))
#define TESTANY(mask, var) (((mask) & (var)) != 0)

#define ARG_NUMBER_FILE 2
#define INDEX_WHERE_IS_FILE_NAME 1

/* global_fp */
FILE * global_fp;

static void
instrace( app_pc received_app_pc,int size){

    UniqueInstr* instr;

    // dr_printf(" > 0x%lx arthur\n",*received_app_pc);

    instr = get_instr( (ptr_uint_t) received_app_pc , size);
    instr_inc(instr, 1);

}

static dr_emit_flags_t
event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *inst,
                      bool for_trace, bool translating, void *user_data)
{
    int ins_size;
    app_pc instruction_app_pc;  

    drmgr_disable_auto_predication(drcontext, bb);
    if (!drmgr_is_first_instr(drcontext, inst))
        return DR_EMIT_DEFAULT;

    instr_t *instr;

    for (instr = instrlist_first(bb); instr != NULL;
        instr = instr_get_next(instr)) {
        instruction_app_pc = instr_get_app_pc(instr);
        ins_size = decode_sizeof( drcontext, instruction_app_pc , NULL, NULL );
        dr_insert_clean_call(drcontext, bb, instr, (void *)instrace,
                        false /* save fpstate */, 2, OPND_CREATE_INT64(instruction_app_pc), OPND_CREATE_INT32(ins_size));
    }

    return DR_EMIT_DEFAULT;
}


/* ================================================== */
/* EXIT Funtions                                      */
/* ================================================== */

static void
event_exit(void)
{
    if (!drmgr_unregister_bb_insertion_event(event_app_instruction) || drreg_exit() != DRREG_SUCCESS)
    DR_ASSERT(false);
    drx_exit();
    drmgr_exit();

    dump_instrs();
    destroy_instrs_pool();
}


static void
event_exit_in_file(void)
{
    if (!drmgr_unregister_bb_insertion_event(event_app_instruction) || drreg_exit() != DRREG_SUCCESS)
    DR_ASSERT(false);
    drx_exit();
    drmgr_exit();

    dump_instrs_in_file(global_fp);
    destroy_instrs_pool();
    fclose(global_fp);
}

DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{
    FILE * fp;
    fp = NULL;

    if( argc == ARG_NUMBER_FILE ){
        fp = fopen(argv[INDEX_WHERE_IS_FILE_NAME], "w");//opening file.
    }

     init_instrs_pool();

    /* We need 2 reg slots beyond drreg's eflags slots => 3 slots */
    drreg_options_t ops = { sizeof(ops), 3, false };
    dr_set_client_name("DynamoRIO Sample Client 'bbl_count'",
                       "http://dynamorio.org/issues");
    
    if (!drmgr_init() || drreg_init(&ops) != DRREG_SUCCESS)
        DR_ASSERT(false);

    if (fp == NULL) {
        /* register events */
        dr_register_exit_event(event_exit);
    }else{
        global_fp = fp;
        dr_register_exit_event(event_exit_in_file);
    }

    /* register events */
    if (!drmgr_register_bb_instrumentation_event(NULL, event_app_instruction, NULL))
        DR_ASSERT(false);

    /* make it easy to tell, by looking at log file, which client executed */
    dr_log(NULL, DR_LOG_ALL, 1, "Client 'bbl_count' initializing\n");

}
