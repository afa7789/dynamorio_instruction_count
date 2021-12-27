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

/* we only have a global count */
static int global_count;


static void
instrace(void *drcontext, app_pc * received_app_pc){

    UniqueInstr* instr;
    int ins_size;

    ins_size = decode_sizeof( drcontext, received_app_pc , NULL, NULL );
    instr = get_instr( (ptr_uint_t) received_app_pc , ins_size);
    instr_inc(instr, 1);

}


static dr_emit_flags_t
event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *inst,
                      bool for_trace, bool translating, void *user_data)
{

    app_pc *instruction_app_pc;  

    drmgr_disable_auto_predication(drcontext, bb);
    if (!drmgr_is_first_instr(drcontext, inst))
        return DR_EMIT_DEFAULT;

    for (instr = instrlist_first(bb), num_instrs = 0; instr != NULL;
        instr = instr_get_next(instr)) {
        instruction_app_pc = appinstr_get_app_pc(instr)
        instrace(drcontext,instruction_app_pc)
    }

    return DR_EMIT_DEFAULT;
}

DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{
    FILE * fp;
    fp = NULL;

    if( argc == ARG_NUMBER_FILE ){
        // printf("\n - %s - \n",argv[INDEX_WHERE_IS_FILE_NAME]);
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

/* ================================================== */
/* EXIT Funtions
/* ================================================== */

static void
event_exit(void)
{
    char msg[512];
    int len;
    len = dr_snprintf(msg, sizeof(msg) / sizeof(msg[0]),
                      "Instrumentation results:\n"
                      "%10d basic block executions\n"
                      global_count);
    DR_ASSERT(len > 0);
    NULL_TERMINATE(msg);
    DISPLAY_STRING(msg);

    drx_exit();
    drreg_exit();
    drmgr_exit();

    dump_instrs();
    destroy_instrs_pool();
}


static void
event_exit_in_file(void)
{
    char msg[512];
    int len;
    len = dr_snprintf(msg, sizeof(msg) / sizeof(msg[0]),
                      "Instrumentation results:\n"
                      "%10d basic block executions\n"
                      global_count);
    DR_ASSERT(len > 0);
    NULL_TERMINATE(msg);
    DISPLAY_STRING(msg);

    drx_exit();
    drreg_exit();
    drmgr_exit();

    dump_instrs_in_file(global_fp);
    destroy_instrs_pool();
    fclose(global_fp);
}