// sys/threads.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// platform independent wrapper around the system's thread functionality

#ifndef DROMAIUS_SYS_THREADS_H
#define DROMAIUS_SYS_THREADS_H

#ifdef PLATFORM_LINUX
	#define DMS_THREADS_C11
#endif // (platform detection)

//////////////////////////////////////////////////////////////////////////////
//
// C11 threads
//

#ifdef DMS_THREADS_C11

#include <threads.h>
#include <stdbool.h>

typedef thrd_t	thread_t;
typedef mtx_t	mutex_t;
typedef cnd_t	cond_t;

typedef int (*thread_func_t)(void *);

bool thread_create_joinable(thread_t *thread, thread_func_t func, void *arg);
bool thread_join(thread_t thr, int *res);

#define mutex_init_plain(m)		mtx_init((m), mtx_plain)
#define mutex_lock(m)			mtx_lock((m))
#define mutex_unlock(m)			mtx_unlock((m))

#define cond_init(c)			cnd_init((c))
#define cond_wait(c,m)			cnd_wait((c),(m))
#define cond_signal(c)			cnd_signal((c))

#endif // DMS_THREADS_C11

#endif // DROMAIUS_SYS_THREADS_H
