/*
 *	author: muggle wei <mugglewei@gmail.com>
 *
 *	Use of this source code is governed by the MIT license that can be
 *	found in the LICENSE file.
 */

#include "os.h"
#include "muggle/c/base/err.h"

#if MUGGLE_PLATFORM_WINDOWS

#include <windows.h>

int muggle_os_process_path(char *path, unsigned int size)
{
	// convert to unicode characters
	WCHAR unicode_buf[MUGGLE_MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, unicode_buf, MUGGLE_MAX_PATH);

	// convert to utf8
	WideCharToMultiByte(CP_UTF8, 0, unicode_buf, -1, path, size - 1, NULL, FALSE);

	return MUGGLE_OK;
}

int muggle_os_curdir(char *path, unsigned int size)
{
	// TODO:
	return MUGGLE_ERR_TODO;
}

int muggle_os_chdir(const char *path)
{
	// TODO:
	return MUGGLE_ERR_TODO;
}

#else

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int muggle_os_process_path(char *path, unsigned int size)
{
	char sz_tmp[64], buf[MUGGLE_MAX_PATH];
	ssize_t len;

	snprintf(sz_tmp, 63, "/proc/%ld/exe", (long)getpid());
	len = readlink(sz_tmp, path, size - 1);
	if (len >= 0)
	{
		path[len] = '\0';
		return MUGGLE_OK;
	}

	return MUGGLE_ERR_SYS_CALL;
}

int muggle_os_curdir(char *path, unsigned int size)
{
	return getcwd(path, (size_t)size) != NULL ? MUGGLE_OK : MUGGLE_ERR_SYS_CALL;
}

int muggle_os_chdir(const char *path)
{
	return chdir(path) == 0 ? MUGGLE_OK : MUGGLE_ERR_SYS_CALL;
}

#endif