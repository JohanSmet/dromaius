// sys/atomics.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// platform independent wrapper for atomic operations

#ifndef DROMAIUS_SYS_ATOMICS_H
#define DROMAIUS_SYS_ATOMICS_H

#undef DMS_ATOMICS_C11
#undef DMS_ATOMICS_WIN32

// expect C11 atomics to be supported, make exceptions for specific platforms if not
#if defined (PLATFORM_WINDOWS)
	#define DMS_ATOMICS_WIN32
#else
	#if defined (__STDC_NO_ATOMICS__)
		#error "No support for atomics on this platform"
	#else
		#define DMS_ATOMICS_C11
	#endif
#endif

//////////////////////////////////////////////////////////////////////////////
//
// C11 atomics
//

#ifdef DMS_ATOMICS_C11

#include <stdint.h>
#include <stdatomic.h>

typedef atomic_flag				flag_t;
typedef atomic_uint_least32_t	atomic_uint32_t;


#define FLAG_INIT	ATOMIC_FLAG_INIT

static inline void flag_acquire_lock(volatile flag_t *flag) {
	while (atomic_flag_test_and_set_explicit(flag, memory_order_acquire));
}

static inline void flag_release_lock(volatile flag_t *flag) {
	atomic_flag_clear_explicit(flag, memory_order_release);
}

static inline uint_least32_t atomic_exchange_uint32(volatile atomic_uint32_t *obj, uint32_t desired) {
	return atomic_exchange(obj, desired);
}

static inline uint_least32_t atomic_load_uint32(volatile atomic_uint32_t *obj) {
	return atomic_load(obj);
}

#endif // DMS_ATOMICS_C11

//////////////////////////////////////////////////////////////////////////////
//
// Win32 atomics
//

#ifdef DMS_ATOMICS_WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef LONG		flag_t;
typedef DWORD		atomic_uint32_t;

#define FLAG_INIT	0l

static inline void flag_acquire_lock(volatile flag_t *flag) {
	while (InterlockedExchangeAcquire(flag, 1) == 1);
}

static inline void flag_release_lock(volatile flag_t *flag) {
	InterlockedExchange(flag, 0);
}

static inline uint_least32_t atomic_exchange_uint32(volatile atomic_uint32_t *obj, uint32_t desired) {
	return InterlockedExchange((volatile LONG *) obj, desired);
}

static inline uint_least32_t atomic_load_uint32(volatile atomic_uint32_t *obj) {
	return InterlockedOr((volatile LONG *) obj, 0);
}

#endif // DMS_ATOMICS_WIN32

#endif // DROMAIUS_SYS_ATOMICS_H
