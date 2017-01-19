#include "muggle/base/base.h"

typedef void(*emptyFunc)();

static MutexHandle mtx;
static int thread_loops = 0;

void TestFileHandle()
{
	FileHandle fh;
	int flags, attr;
	char buf1[] = "hello world", buf2[64] = { 0 };
	long num_read;

	flags = 
		MUGGLE_FILE_WRITE | MUGGLE_FILE_READ | MUGGLE_FILE_APPEND | 
		MUGGLE_FILE_CREAT | MUGGLE_FILE_EXCL;
	attr = MUGGLE_FILE_ATTR_USER_READ | MUGGLE_FILE_ATTR_USER_WRITE;
	FileHandleOpen(&fh, "tmp.txt", flags, attr);
	if (!FileHandleIsValid(&fh))
	{
		flags &= ~MUGGLE_FILE_EXCL;
		flags |= MUGGLE_FILE_TRUNC;
		FileHandleOpen(&fh, "tmp.txt", flags, attr);
		if (!FileHandleIsValid(&fh))
		{
			MUGGLE_DEBUG_ERROR("Can't get file handle of tmp.txt\n");
			return;
		}
		else
		{
			MUGGLE_DEBUG_INFO("Open tmp.txt and truncate\n");
		}
	}
	else
	{
		MUGGLE_DEBUG_INFO("I create tmp.txt\n");
	}

	if (FileHandleWrite(&fh, buf1, (long)strlen(buf1)) == -1)
	{
		MUGGLE_DEBUG_ERROR("Failed write into file handle\n");
		return;
	}

	if (!FileHandleClose(&fh))
	{
		MUGGLE_DEBUG_ERROR("Failed close file handle\n");
		return;
	}

	flags &= ~(MUGGLE_FILE_EXCL | MUGGLE_FILE_TRUNC);
	FileHandleOpen(&fh, "tmp.txt", flags, attr);
	if (!FileHandleIsValid(&fh))
	{
		MUGGLE_DEBUG_ERROR("Can't open tmp.txt\n");
		return;
	}

	num_read = FileHandleRead(&fh, buf2, 64);
	if (num_read == -1)
	{
		MUGGLE_DEBUG_ERROR("Failed read data from file handle\n");
		return;
	}
	buf2[num_read] = '\0';
	MUGGLE_INFO("Read data: %s\n", buf2);

	if (FileHandleSeek(&fh, 6L, MUGGLE_FILE_SEEK_BEGIN) == -1)
	{
		MUGGLE_DEBUG_ERROR("Failed seek in file handle\n");
		return;
	}
	num_read = FileHandleRead(&fh, buf2, 64);
	if (num_read == -1)
	{
		MUGGLE_DEBUG_ERROR("Failed read data file handle\n");
		return;
	}
	buf2[num_read] = '\0';
	MUGGLE_INFO("Read data: %s\n", buf2);

	if (!FileHandleClose(&fh))
	{
		MUGGLE_DEBUG_ERROR("Failed close file handle\n");
		return;
	}
}

void* threadFunc(void *args)
{
	int idx = *(int*)args;

	MUGGLE_INFO("#%d thread run\n", idx);

	for (int i = 0; i < 10000; ++i)
	{
		MutexLock(&mtx);

		++thread_loops;

		MutexUnLock(&mtx);
	}	

	return NULL;
}

void TestThread()
{
	ThreadHandle th[5];
	int idx[5] = { 0, 1, 2, 3, 4};
	int i;

	if (!MutexInit(&mtx))
	{
		MUGGLE_DEBUG_ERROR("Failed init mutex\n");
		return;
	}

	for (i = 0; i < 5; ++i)
	{
		if (ThreadCreate(&th[i], NULL, threadFunc, (void*)&idx[i]))
		{
			MUGGLE_DEBUG_INFO("start #%d thread\n", i);
		}
		else
		{
			MUGGLE_DEBUG_ERROR("Failed create thread\n");
		}
	}

	for (i = 0; i < 5; ++i)
	{
		if (ThreadWaitExit(&th[i]))
		{
			MUGGLE_DEBUG_INFO("#%d thread joined\n", i);
		}
		else
		{
			MUGGLE_DEBUG_ERROR("Failed create thread\n");
		}
	}

	if (!MutexDestroy(&mtx))
	{
		MUGGLE_DEBUG_ERROR("Failed destroy mutex\n");
		return;
	}

	MUGGLE_INFO("thread loop count: %d\n", thread_loops);
}

int main(int argc, char *argv[])
{
	void* dllHandle;
	void* funcHandle;
	emptyFunc func;
	deltaTime dt;
	double ms;
	char buf[MUGGLE_MAX_PATH];
	char dll_name[] = "RunTestEmpty", dll_name2[] = "../lib/libRunTestEmpty";
	size_t len;
	int ival;
	unsigned int uival;
	long lval;
	unsigned long ulval;
	float fval;
	int i;

	// platform macro
#if MUGGLE_PLATFORM_WIN64
	MUGGLE_DEBUG_INFO("Win64\n");
#elif MUGGLE_PLATFORM_WINDOWS
	MUGGLE_DEBUG_INFO("Win32\n");
#elif MUGGLE_PLATFORM_LINUX
	MUGGLE_DEBUG_INFO("Linux\n");
#elif MUGGLE_PLATFORM_APPLE
	MUGGLE_DEBUG_INFO("Apple\n");
#elif MUGGLE_PLATFORM_UNIX
	MUGGLE_DEBUG_INFO("Unix\n");
#endif

	// log
	MUGGLE_DEBUG_INFO("Hello world\n");

	// log default init
	LogDefaultInit("logout.txt", 1);

	// test log color
	for (i = 0; i < 3; ++i)
	{
		MUGGLE_INFO("Info\n");
		MUGGLE_WARNING("Warning\n");
	}

	// sleep and delta time
	MUGGLE_DEBUG_INFO("I will sleep 3000ms\n");

	DTStart(&dt);
	MSleep(3000);
	DTEnd(&dt);

	ms = DTGetElapsedMilliseconds(&dt);
	MUGGLE_DEBUG_INFO("real sleep time: %fms\n", ms);

	// string convert
	if (StrToi("-1024", &ival, 0))
	{
		MUGGLE_DEBUG_INFO("int: %d\n", ival);
	}
	if (StrToui("1024", &uival, 0))
	{
		MUGGLE_DEBUG_INFO("uint: %d\n", uival);
	}
	if (StrTol("-1024", &lval, 0))
	{
		MUGGLE_DEBUG_INFO("long: %ld\n", lval);
	}
	if (StrToul("102400", &ulval, 0))
	{
		MUGGLE_DEBUG_INFO("ulong: %uld\n", ulval);
	}
	if (StrTof("5.2", &fval))
	{
		MUGGLE_DEBUG_INFO("float: %f\n", fval);
	}

	// file and dll
	FileGetProcessPath(buf);
	FileGetDirectory(buf, buf);
	len = strlen(buf);
	memcpy(buf + len, dll_name, strlen(dll_name));
	buf[len+strlen(dll_name)] = '\0';

	dllHandle = DllLoad(buf);
	if (dllHandle == NULL)
	{
		memcpy(buf + len, dll_name2, strlen(dll_name2));
		buf[len+strlen(dll_name2)] = '\0';
		dllHandle = DllLoad(buf);
	}
	if (dllHandle == NULL)
	{
		MUGGLE_DEBUG_ERROR("Can't find demo dll\n");
		exit(EXIT_FAILURE);
	}

	funcHandle = DllQueryFunc(dllHandle, "RunTestEmptyFunc");
	if (funcHandle == NULL)
	{
		MUGGLE_DEBUG_ERROR("Can't find function in dll\n");
		exit(EXIT_FAILURE);
	}

	func = (emptyFunc)funcHandle;
	func();

	if (!DllFree(dllHandle))
	{
		MUGGLE_DEBUG_ERROR("Failed in free dll\n");
		exit(EXIT_FAILURE);
	}

	// file handle
	MUGGLE_DEBUG_INFO("\n===== Test File Handle\n");
	TestFileHandle();

	// thread
	MUGGLE_DEBUG_INFO("\n===== Test Thread\n");
	TestThread();

	return 0;
}