// sys/threads.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// platform independent wrapper around the system's thread functionality

#ifndef DROMAIUS_SYS_THREADS_H
#define DROMAIUS_SYS_THREADS_H

#undef DMS_THREADS_C11
#undef DMS_THREADS_POSIX
#undef DMS_THREADS_WIN32

#ifdef PLATFORM_LINUX
	#ifdef __STDC_NO_THREADS__
		#define DMS_THREADS_POSIX
	#else
		#define DMS_THREADS_C11
	#endif
#elif defined(PLATFORM_EMSCRIPTEN)
	#define DMS_THREADS_POSIX
#elif defined(PLATFORM_DARWIN)
	#define DMS_THREADS_POSIX
#elif defined(PLATFORM_WINDOWS)
	#define DMS_THREADS_WIN32
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

//////////////////////////////////////////////////////////////////////////////
//
// Win32 threads
//

#ifdef DMS_THREADS_WIN32

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef void *				thread_t;
typedef CRITICAL_SECTION	mutex_t;
typedef CONDITION_VARIABLE	cond_t;

typedef int (*thread_func_t)(void *);

bool thread_create_joinable(thread_t *thread, thread_func_t func, void *arg);
bool thread_join(thread_t thread, int *thread_res);

bool mutex_init_plain(mutex_t *mutex);
bool mutex_destroy(mutex_t *mutex);
bool mutex_lock(mutex_t *mutex);
bool mutex_unlock(mutex_t *mutex);

bool cond_init(cond_t *cond);
bool cond_destroy(cond_t* cond);
bool cond_wait(cond_t* cond, mutex_t* mutex);
bool cond_signal(cond_t* cond);

#endif // DMS_THREADS_WIN32


#endif // DROMAIUS_SYS_THREADS_H
