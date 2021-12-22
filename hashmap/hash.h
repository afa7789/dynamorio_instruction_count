#ifndef _HASH_H
#define _HASH_H

#include <sys/types.h>

//#define DEFAULT_POOL_SIZE 262144 // 256k instructions
#define DEFAULT_POOL_SIZE 4

typedef unsigned long Addr;
typedef struct _UniqueInstr UniqueInstr;

void init_instrs_pool(void);
void destroy_instrs_pool(void);
UniqueInstr* get_instr(Addr addr, u_int64_t size);
UniqueInstr* find_instr(Addr addr);
Addr instr_addr(UniqueInstr* instr);
int instr_size(UniqueInstr* instr);
u_int64_t instr_count(UniqueInstr* instr);
void instr_inc(UniqueInstr* instr, u_int64_t count);
int instrs_cmp(UniqueInstr* i1, UniqueInstr* i2);
void print_instr(UniqueInstr* instr);
void dump_instrs(void);
void print_instr_in_file(UniqueInstr* instr,FILE* fp);
void dump_instrs_in_file(FILE* fp);
#endif