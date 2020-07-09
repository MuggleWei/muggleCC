/*
 *	author: muggle wei <mugglewei@gmail.com>
 *
 *	Use of this source code is governed by the MIT license that can be
 *	found in the LICENSE file.
 */

#ifndef MUGGLE_C_THREADSAFE_MEMORY_POOL_H_
#define MUGGLE_C_THREADSAFE_MEMORY_POOL_H_

#include "muggle/c/base/macro.h"
#include "muggle/c/base/atomic.h"

EXTERN_C_BEGIN

struct muggle_ts_memory_pool;

typedef struct muggle_ts_memory_pool_head
{
	MUGGLE_STRUCT_CACHE_LINE_PADDING(0);
	struct muggle_ts_memory_pool *pool;
}muggle_ts_memory_pool_head_t;

typedef struct muggle_ts_memory_pool_head_ptr
{
	MUGGLE_STRUCT_CACHE_LINE_PADDING(0);
	muggle_ts_memory_pool_head_t *ptr;
}muggle_ts_memory_pool_head_ptr_t;

typedef struct muggle_ts_memory_pool
{
	muggle_atomic_int                capacity;
	muggle_atomic_int                block_size;
	void                             *data;
	muggle_ts_memory_pool_head_ptr_t *ptrs;

	MUGGLE_STRUCT_CACHE_LINE_PADDING(0);
	muggle_atomic_int alloc_cursor;
	MUGGLE_STRUCT_CACHE_LINE_PADDING(1);
	muggle_atomic_int free_cursor;
}muggle_ts_memory_pool_t;

/*
 * init muggle thread safe memory pool
 * @pool: pointer to ts_memory_pool
 * @capacity: expected capacity of pool
 * @data_size: user data size
 * RETURN: 0 - success, otherwise failed and return MUGGLE_ERR_*
 * */
MUGGLE_CC_EXPORT
int muggle_ts_memory_pool_init(muggle_ts_memory_pool_t *pool, muggle_atomic_int capacity, muggle_atomic_int data_size);

MUGGLE_CC_EXPORT
void muggle_ts_memory_pool_destroy(muggle_ts_memory_pool_t *pool);

MUGGLE_CC_EXPORT
void* muggle_ts_memory_pool_alloc(muggle_ts_memory_pool_t *pool);

MUGGLE_CC_EXPORT
void muggle_ts_memory_pool_free(void *data);

EXTERN_C_END

#endif