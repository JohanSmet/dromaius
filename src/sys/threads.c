// sys/threads.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// platform independent wrapper around the system's thread functionality

#include "threads.h"

//////////////////////////////////////////////////////////////////////////////
//
// C11 threads
//

#ifdef DMS_THREADS_C11

bool thread_create_joinable(thread_t *thread, thread_func_t func, void *arg) {
	int res = thrd_create(thread, func, arg);
	return res == thrd_success;
}

bool thread_join(thread_t thread, int *thread_res) {
	int res = thrd_join(thread, thread_res);
	return res == 0;
}

#endif // DMS_THREADS_C11

//////////////////////////////////////////////////////////////////////////////
//
// posix threads
//

#ifdef DMS_THREADS_POSIX

bool thread_create_joinable(thread_t *thread, thread_func_t func, void *arg) {
	int res = pthread_create(thread, NULL, (void *(*)(void *)) func, arg);
	return res == 0;
}

bool thread_join(thread_t thread, int *thread_res) {
	int res = pthread_join(thread, (void **) thread_res);
	return res == 0;
}

#endif // DMS_THREADS_POSIX

//////////////////////////////////////////////////////////////////////////////
//
// win32 threads
//

#ifdef DMS_THREADS_WIN32


bool thread_create_joinable(thread_t* thread, thread_func_t func, void* arg) {
	*thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) func, arg, 0, NULL);
	return *thread != 0;
}

bool thread_join(thread_t thread, int* thread_res) {
	if (WaitForSingleObject(thread, INFINITE) == WAIT_FAILED) {
		return false;
	}

	if (thread_res != NULL) {
		DWORD result;
		if (!GetExitCodeThread(thread, &result)) {
			return false;
		}
		*thread_res = result;
	}

	CloseHandle(thread);
	return false;
}

bool mutex_init_plain(mutex_t* mutex) {
	InitializeCriticalSection(mutex);
	return true;
}

bool mutex_destroy(mutex_t* mutex) {
	DeleteCriticalSection(mutex);
	return true;
}

bool mutex_lock(mutex_t* mutex) {
	EnterCriticalSection(mutex);
	return true;
}

bool mutex_unlock(mutex_t* mutex) {
	LeaveCriticalSection(mutex);
	return true;
}

bool cond_init(cond_t* cond) {
	InitializeConditionVariable(cond);
	return true;
}

bool cond_destroy(cond_t* cond) {
	cond;		// silence unused variable warning
	return true;
}

bool cond_wait(cond_t* cond, mutex_t* mutex) {
	return SleepConditionVariableCS(cond, mutex, INFINITE) != FALSE;
}

bool cond_signal(cond_t* cond) {
	WakeConditionVariable(cond);
	return true;
}

#endif // DMS_THREADS_WIN32
