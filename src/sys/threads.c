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

