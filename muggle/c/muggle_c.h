/*
 *	author: muggle wei <mugglewei@gmail.com>
 *
 *	Use of this source code is governed by the MIT license that can be
 *	found in the LICENSE file.
 */

#ifndef MUGGLE_C_H_
#define MUGGLE_C_H_

// base c header
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <time.h>

// muggle c base header
#include "muggle/c/base/macro.h"
#include "muggle/c/base/err.h"
#include "muggle/c/base/utils.h"
#include "muggle/c/base/str.h"
#include "muggle/c/base/thread.h"
#include "muggle/c/base/atomic.h"

// memory
#include "muggle/c/memory/memory_pool.h"
#include "muggle/c/memory/sowr_memory_pool.h"

// muggle c time
#include "muggle/c/time/win_gettimeofday.h"

// os
#include "muggle/c/os/dl.h"

// sync
#include "muggle/c/sync/mutex.h"
#include "muggle/c/sync/condition_variable.h"
#include "muggle/c/sync/futex.h"
#include "muggle/c/sync/ringbuffer.h"

#endif
