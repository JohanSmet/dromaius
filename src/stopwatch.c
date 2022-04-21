// stopwatch.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// wall-time interface

#include "stopwatch.h"
#include "crt.h"

#undef DMS_TIMER_WIN32
#undef DMS_TIMER_POSIX

#if defined(PLATFORM_LINUX)
	#define DMS_TIMER_POSIX
#elif defined(PLATFORM_EMSCRIPTEN)
	#define DMS_TIMER_POSIX
#elif defined(PLATFORM_DARWIN)
	#define DMS_TIMER_POSIX
#elif defined(PLATFORM_WINDOWS)
	#define DMS_TIMER_WIN32
#endif // platform detection

#ifdef DMS_TIMER_POSIX
	#include <time.h>
	#include <errno.h>
#endif // DMS_TIMER_POSIX

#ifdef DMS_TIMER_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif // DMS_TIMER_WIN32

//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#ifdef DMS_TIMER_POSIX

static inline void stopwatch_init(void) {
}

static inline int64_t timespec_to_int(struct timespec t) {
	return (int64_t) S_TO_PS(t.tv_sec) + NS_TO_PS(t.tv_nsec);
}

static inline int64_t timestamp_current(void) {
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return timespec_to_int(ts);
}

static inline void stopwatch_platform_sleep(int64_t interval_ps) {
	struct timespec ts, rem;
	ts.tv_sec = interval_ps / S_TO_PS(1);
	ts.tv_nsec = (long) ((interval_ps - S_TO_PS(ts.tv_sec)) / 1000);

	int result = nanosleep(&ts, &rem);

	while (result != 0 && errno == EINTR) {
		ts = rem;
		result = nanosleep(&ts, &rem);
	}
}

#endif // DMS_TIMER_POSIX

#ifdef DMS_TIMER_WIN32

static LARGE_INTEGER qpc_ticks_per_second = { 0 };
static LARGE_INTEGER qpc_initial_count = { 0 };

static inline void stopwatch_init(void) {
	if (qpc_ticks_per_second.QuadPart == 0 && !QueryPerformanceFrequency(&qpc_ticks_per_second)) {
		// FIXME: error reporting
	}
	QueryPerformanceCounter(&qpc_initial_count);
}

static inline int64_t timestamp_current(void) {
	LARGE_INTEGER count;

	QueryPerformanceCounter(&count);
	return (count.QuadPart - qpc_initial_count.QuadPart) * (1000000000000ll / qpc_ticks_per_second.QuadPart);
}

static inline void stopwatch_platform_sleep(int64_t interval_ps) {
	Sleep((DWORD) PS_TO_MS(interval_ps));
}

#endif // DMS_TIMER_WIN32

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Stopwatch *stopwatch_create() {
	stopwatch_init();

	Stopwatch *stopwatch = (Stopwatch *) dms_calloc(1, sizeof(Stopwatch));
	return stopwatch;
}

void stopwatch_destroy(Stopwatch *stopwatch) {
	assert(stopwatch);
	dms_free(stopwatch);
}

void stopwatch_start(Stopwatch *stopwatch) {
	assert(stopwatch);

	if (!stopwatch->running) {
		stopwatch->running = true;
		stopwatch->start_timestamp = timestamp_current();
	}
}

void stopwatch_stop(Stopwatch *stopwatch) {
	assert(stopwatch);
	stopwatch->running = false;
}

int64_t stopwatch_time_elapsed_ps(Stopwatch *stopwatch) {
	assert(stopwatch);

	if (stopwatch->running) {
		return timestamp_current() - stopwatch->start_timestamp;
	} else {
		return 0;
	}
}

void stopwatch_sleep(int64_t interval_ps) {
	stopwatch_platform_sleep(interval_ps);
}
