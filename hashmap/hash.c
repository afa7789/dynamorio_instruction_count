#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "hash.h"

struct _UniqueInstr {
    Addr addr;
    int size;
    int count;

    UniqueInstr* chain;
};

typedef struct _instrs_hash instrs_hash;
struct _instrs_hash {
	u_int64_t size, entries;
	UniqueInstr** table;
};

instrs_hash pool;

static
void delete_instr(UniqueInstr* instr) {
	assert(instr != 0);
	free(instr);
}

static inline
u_int64_t instrs_hash_idx(Addr addr, u_int64_t size) {
	return addr % size;
}

static
void resize_instrs_pool(void) {
    u_int64_t i, new_size, conflicts1 = 0;
    UniqueInstr **new_table, *curr, *next;
    u_int64_t new_idx;

	// increase table by 50%.
    new_size  = (u_int64_t) (1.5f * pool.size);
    new_table = (UniqueInstr**)
                    malloc(new_size * sizeof(UniqueInstr*));
    memset(new_table, 0, (new_size * sizeof(UniqueInstr*)));

    for (i = 0; i < pool.size; i++) {
		if (pool.table[i] == 0)
			continue;

		curr = pool.table[i];
		while (curr != 0) {
			next = curr->chain;

			new_idx = instrs_hash_idx(curr->addr, new_size);

			curr->chain = new_table[new_idx];
			new_table[new_idx] = curr;
			if (curr->chain)
				conflicts1++;

			curr = next;
		}
    }

    free(pool.table);

    pool.size  = new_size;
    pool.table = new_table;
}

static
UniqueInstr* lookup_instr(Addr addr) {
	UniqueInstr* instr;
	u_int64_t idx;

	assert(addr != 0);

	idx = instrs_hash_idx(addr, pool.size);
	instr = pool.table[idx];

	while (instr) {
		if (instr->addr == addr)
			break;

		instr = instr->chain;
	}

	return instr;
}

void init_instrs_pool(void) {
	u_int64_t size;

	pool.size = DEFAULT_POOL_SIZE;
	pool.entries = 0;

	size = pool.size * sizeof(UniqueInstr*);
	pool.table = (UniqueInstr**) malloc(size);
	memset(pool.table, 0, size);
}

void destroy_instrs_pool(void) {
	u_int64_t i;

	for (i = 0; i < pool.size; i++) {
		UniqueInstr* instr = pool.table[i];
		while (instr) {
			UniqueInstr* next = instr->chain;
			delete_instr(instr);
			instr = next;

			pool.entries--;
		}
	}

	assert(pool.entries == 0);

	free(pool.table);
	pool.table = 0;
}

UniqueInstr* get_instr(Addr addr, u_int64_t size) {
	UniqueInstr* instr = lookup_instr(addr);
	if (instr) {
		assert(instr->addr == addr);
		if (size != 0) {
			if (instr->size == 0) {
				instr->size = size;
			} else {
				assert(instr->size == size);
			}
		}
	} else {
		u_int64_t idx;

		/* check fill degree of instructions pool and resize if needed (>80%) */
		pool.entries++;
		if (10 * pool.entries / pool.size > 8)
			resize_instrs_pool();

		// Create the instruction.
		instr = (UniqueInstr*) malloc(sizeof(UniqueInstr));
		memset(instr, 0, sizeof(UniqueInstr));
		instr->addr = addr;
		instr->size = size;
    
		/* insert into instructions pool */
		idx = instrs_hash_idx(addr, pool.size);
		instr->chain = pool.table[idx];
		pool.table[idx] = instr;
	}

	return instr;
}

UniqueInstr* find_instr(Addr addr) {
	return lookup_instr(addr);
}

Addr instr_addr(UniqueInstr* instr) {
	assert(instr != 0);
	return instr->addr;
}

int instr_size(UniqueInstr* instr) {
	assert(instr != 0);
	return instr->size;
}

u_int64_t instr_count(UniqueInstr* instr) {
    assert(instr != 0);
    return instr->count;
}

void instr_inc(UniqueInstr* instr, u_int64_t count) {
    assert(instr != 0);
    instr->count += count;
}

int instrs_cmp(UniqueInstr* i1, UniqueInstr* i2) {
	return i1 && i2 && i1->addr == i2->addr && i1->size == i2->size;
}

void print_instr(UniqueInstr* instr) {
	assert(instr != 0);

	printf("0x%lx:%d:%u\n", instr->addr, instr->size, instr->count);
}

void print_instr_in_file(UniqueInstr* instr,FILE *fp) {
	assert(instr != 0);

	fprintf(fp,"0x%lx:%d:%u\n", instr->addr, instr->size, instr->count);
}

void dump_instrs(void) {
    u_int64_t idx, size;
    UniqueInstr* instr;

    size = pool.size;
    for (idx = 0; idx < size; idx++) {
        instr = pool.table[idx];
        while (instr) {
            print_instr(instr);
            instr = instr->chain;
        }
    }
}

void dump_instrs_in_file(FILE *fp) {
    u_int64_t idx, size;
    UniqueInstr* instr;

    size = pool.size;
    for (idx = 0; idx < size; idx++) {
        instr = pool.table[idx];
        while (instr) {
            print_instr_in_file(instr,fp);
            instr = instr->chain;
        }
    }
}