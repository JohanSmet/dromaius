// sys/threads.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// platform independent wrapper around the system's thread functionality

#ifndef DROMAIUS_SYS_THREADS_H
#define DROMAIUS_SYS_THREADS_H

#ifdef PLATFORM_LINUX
	#define DMS_THREADS_C11
	#undef DMS_THREADS_POSIX
#elif defined(PLATFORM_EMSCRIPTEN)
	#undef DMS_THREADS_C11
	#define DMS_THREADS_POSIX
#elif defined(PLATFORM_DARWIN)
	#undef DMS_THREADS_C11
	#define DMS_THREADS_POSIX
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

//////////////////////////////////////////////////////////////////////////////
//
// posix threads
//

#ifdef DMS_THREADS_POSIX

#include <pthread.h>
#include <stdbool.h>

typedef pthread_t			thread_t;
typedef pthread_mutex_t		mutex_t;
typedef pthread_cond_t		cond_t;

typedef int (*thread_func_t)(void *);

bool thread_create_joinable(thread_t *thread, thread_func_t func, void *arg);
bool thread_join(thread_t thread, int *thread_res);

#define mutex_init_plain(m)		pthread_mutex_init((m), NULL)
#define	mutex_lock(m)			pthread_mutex_lock((m))
#define	mutex_unlock(m)			pthread_mutex_unlock((m))

#define cond_init(c)			pthread_cond_init((c), NULL)
#define cond_wait(c,m)			pthread_cond_wait((c),(m))
#define cond_signal(c)			pthread_cond_signal((c))

#endif // DMS_THREADS_POSIX

#endif // DROMAIUS_SYS_THREADS_H
