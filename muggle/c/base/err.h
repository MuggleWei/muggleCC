/*
 *	author: muggle wei <mugglewei@gmail.com>
 *
 *	Use of this source code is governed by the MIT license that can be
 *	found in the LICENSE file.
 */

#ifndef MUGGLE_C_ERR_H_
#define MUGGLE_C_ERR_H_

#include "muggle/c/base/macro.h"

EXTERN_C_BEGIN

enum
{
	eMuggleOk = 0,
	eMuggleErrMemAlloc,
	eMuggleErrInvalidParam,
	eMuggleErrAcqLock,
	eMuggleErrSysCall,
	eMuggleErrMax,
};

EXTERN_C_END

#endif
