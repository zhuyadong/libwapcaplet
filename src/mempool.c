#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mempool.h"

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/onchipmem.h>

struct Library *ocmb;
struct OCMIFace *IOCM = NULL;
#endif

#define MEMORY_POOL_ALLOCED_CONST   ((void *) 0xFFFFFFFFu)

memory_pool_t * memory_pool_create(size_t bs, size_t c)
{
	memory_pool_t *mp = malloc(sizeof(memory_pool_t));
	if (!mp) return NULL;

	mp->block_size = bs;
	mp->count = c;
	mp->pool = NULL;

#ifdef __amigaos4__
	/* NB: This *always* allocates 64K, requests for more than 64K *must not*
	 * be passed to this function. */
	if((ocmb = IExec->OpenResource("onchipmem.resource"))) {
		if((IOCM = (struct OCMIFace *)IExec->GetInterface((struct Library *)ocmb, "main", 1, NULL))) {
			mp->pool = IOCM->ObtainOnChipMem();
			mp->ocm = true;
		}
	}
#endif

	if(mp->pool == NULL) {
		mp->pool = malloc((mp->block_size + sizeof(void *)) * mp->count);
		mp->ocm = false;
	}

	memory_pool_clear(mp);
	mp->empty_blocks = mp->pool;

	return mp;
}

void memory_pool_destroy(memory_pool_t *mp)
{
	if (!mp) return;

	memory_pool_clear(mp);

	if(mp->ocm == true) {
#ifdef __amigaos4__
		IOCM->ReleaseOnChipMem();
		IExec->DropInterface((struct Interface *)IOCM);
#endif
	} else {
		free(mp->pool);
	}
	free(mp);
}

void memory_pool_clear(memory_pool_t *mp)
{
  if (!mp)
    return;

  size_t i;
  void **p;

  for (i = 0; i < mp->count - 1; i++)
  {
    p = (void **) ((uint8_t *) mp->pool + (mp->block_size * (i + 1) +
                   sizeof(void *) * i));
    *p = (uint8_t *) mp->pool + (mp->block_size + sizeof(void *)) * (i + 1);
  }

  p = (void **) ((uint8_t *) mp->pool + (mp->block_size * mp->count +
                 sizeof(void *) * (mp->count - 1)));
  *p = NULL;

  mp->empty_blocks = mp->pool;
}

void memory_pool_dump(memory_pool_t *mp, void (* print_func) (void *value))
{
  printf("start: %p, size: %d, count: %d\n", mp->pool,
         (mp->block_size + sizeof(void *)) * mp->count, mp->count);

  void *block;
  void **next;
  size_t i;

  for (i = 0; i < mp->count; i++)
  {
    block = (void *) ((uint8_t *) mp->pool + (mp->block_size * i) +
                      sizeof(void *) * i);
    next = (void **) ((uint8_t *) block + sizeof(void *));

    printf("block #%i(%p):", i, block);

    if (*next == MEMORY_POOL_ALLOCED_CONST)
    {
      printf(" allocated");

      if (print_func)
      {
        printf(", value: ");
        print_func(block);
      }

      printf("\n");
    } else
    {
      printf(" free, next address %p\n", *next);
    }
  }
}

void * memory_pool_alloc(memory_pool_t *mp)
{
  void *p;

  if (mp->empty_blocks)
  {
    p = mp->empty_blocks;
    mp->empty_blocks = * (void **) ((uint8_t *) mp->empty_blocks +
                                    mp->block_size);
    *(void **) ((uint8_t *) p + mp->block_size) = MEMORY_POOL_ALLOCED_CONST;
    return p;
  } else
  {
    return NULL;
  }
}

bool memory_pool_free(memory_pool_t *mp, void *p)
{
  if (p && (p >= mp->pool) && (p <= (void *) ((uint8_t *) mp->pool +
      (mp->block_size + sizeof(void *)) * mp->count)))
  {
    *(void **) ((uint8_t *) p + mp->block_size) = mp->empty_blocks;
    mp->empty_blocks = p;
	return true;
  } else {
    return false;
  }
}

