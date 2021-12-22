#include "hash.h"

int main(int argc, char* argv[]) {
    UniqueInstr* instr;

    init_instrs_pool();

    instr = get_instr(0x1234, 4);
    instr_inc(instr, 1);

    instr = get_instr(0x5678, 2);
    instr_inc(instr, 1);
    instr_inc(instr, 1);

    instr = get_instr(0xabcd, 8);
    instr_inc(instr, 1);
    instr_inc(instr, 1);

    instr = get_instr(0x142f, 4);
    instr_inc(instr, 1);
    instr_inc(instr, 1);
    instr_inc(instr, 1);

    dump_instrs();

    destroy_instrs_pool();

    return 0;
}