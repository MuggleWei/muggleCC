/*
 *	author: muggle wei <mugglewei@gmail.com>
 *
 *	Use of this source code is governed by the MIT license that can be
 *	found in the LICENSE file.
 */

#ifndef MUGGLE_C_DLL_H_
#define MUGGLE_C_DLL_H_

#include "muggle/base_c/macro.h"
#include <stdbool.h>

EXTERN_C_BEGIN

MUGGLE_BASE_C_EXPORT void* DllLoad(const char* name);
MUGGLE_BASE_C_EXPORT void* DllQueryFunc(void* dll, const char* func_name);
MUGGLE_BASE_C_EXPORT bool DllFree(void* dll);

EXTERN_C_END

#endif