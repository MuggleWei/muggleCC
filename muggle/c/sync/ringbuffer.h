/*
 *	author: muggle wei <mugglewei@gmail.com>
 *
 *	Use of this source code is governed by the MIT license that can be
 *	found in the LICENSE file.
 */

#ifndef MUGGLE_C_RINGBUFFER_H_
#define MUGGLE_C_RINGBUFFER_H_

#include "muggle/c/base/macro.h"
#include "muggle/c/base/atomic.h"
#include "muggle/c/sync/mutex.h"
#include "muggle/c/sync/condition_variable.h"

EXTERN_C_BEGIN

enum
{
	MUGGLE_RINGBUFFER_FLAG_WRITE_LOCK = 0x00, // default, write with write lock
	MUGGLE_RINGBUFFER_FLAG_READ_ALL   = 0x00, // default, every reader get all message from all writer
	MUGGLE_RINGBUFFER_FLAG_READ_WAIT  = 0x00, // default, if no message in ring, reader wait

	MUGGLE_RINGBUFFER_FLAG_SINGLE_WRITER = 0x01,   // user guarantee only one writer use this ringbuffer
	MUGGLE_RINGBUFFER_FLAG_SINGLE_READER = 0x02,   // user guarantee only one reader use this ringbuffer
	MUGGLE_RINGBUFFER_FLAG_WRITE_BUSY_LOOP = 0x04, // every writer busy loop until write message into ring
	MUGGLE_RINGBUFFER_FLAG_READ_BUSY_LOOP = 0x08,  // reader busy loop until read message from ring
	MUGGLE_RINGBUFFER_FLAG_MSG_READ_ONCE = 0x10,  // every message will only be read once
};

typedef struct muggle_ringbuffer_tag
{
	MUGGLE_STRUCT_CACHE_LINE_PADDING(0);
	muggle_atomic_int capacity;
	MUGGLE_STRUCT_CACHE_LINE_PADDING(1);
	int flag;
	int write_mode;
	int read_mode;
	MUGGLE_STRUCT_CACHE_LINE_PADDING(2);
	muggle_atomic_int next;
	MUGGLE_STRUCT_CACHE_LINE_PADDING(3);
	muggle_atomic_int cursor;
	MUGGLE_STRUCT_CACHE_LINE_PADDING(4);
	muggle_atomic_int read_cursor; // for MUGGLE_RINGBUFFER_FLAG_MSG_READ_ONCE
	MUGGLE_STRUCT_CACHE_LINE_PADDING(5);
	muggle_mutex_t write_mutex;
	MUGGLE_STRUCT_CACHE_LINE_PADDING(6);
	muggle_mutex_t read_mutex;
	MUGGLE_STRUCT_CACHE_LINE_PADDING(7);
	muggle_condition_variable_t read_cv;
	MUGGLE_STRUCT_CACHE_LINE_PADDING(8);
	void **datas;
	MUGGLE_STRUCT_CACHE_LINE_PADDING(9);
}muggle_ringbuffer_t;

MUGGLE_CC_EXPORT
int muggle_ringbuffer_init(muggle_ringbuffer_t *r, muggle_atomic_int capacity, int flag);

MUGGLE_CC_EXPORT
int muggle_ringbuffer_destroy(muggle_ringbuffer_t *r);

MUGGLE_CC_EXPORT
int muggle_ringbuffer_write(muggle_ringbuffer_t *r, void *data);

MUGGLE_CC_EXPORT
void* muggle_ringbuffer_read(muggle_ringbuffer_t *r, muggle_atomic_int idx);

EXTERN_C_END

#endif
