#ifndef MEMPOOL_H
#define MEMPOOL_H 1

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct memory_pool_s
{
	void *pool;
	void *empty_blocks;
	size_t block_size;
	size_t count;
	bool ocm;
} __attribute__ ((__aligned__)) memory_pool_t;

memory_pool_t * memory_pool_create(size_t bs, size_t c);
void memory_pool_destroy(memory_pool_t *mp);
void memory_pool_clear(memory_pool_t *mp);
void memory_pool_dump(memory_pool_t *mp, void (* print_func) (void *value));
void *memory_pool_alloc(memory_pool_t *mp);
bool memory_pool_free(memory_pool_t *mp, void *p);
#endif /* MEMPOOL_H */
