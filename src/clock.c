// clock.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// a configurable oscillator

#include "clock.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#if defined(PLATFORM_LINUX)
	#define DMS_TIMER_POSIX
	#undef DMS_TIMER_WIN32
#elif defined(PLATFORM_EMSCRIPTEN)
	#define DMS_TIMER_POSIX
	#undef DMS_TIMER_WIN32
#elif defined(PLATFORM_DARWIN)
	#define DMS_TIMER_POSIX
	#undef DMS_TIMER_WIN32
#elif defined(PLATFORM_WINDOWS)
	#undef DMS_TIMER_POSIX
	#define DMS_TIMER_WIN32
#endif // platform detection

#ifdef DMS_TIMER_POSIX
	#include <time.h>
#endif // DMS_TIMER_POSIX

#ifdef DMS_TIMER_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif // DMS_TIMER_WIN32

//////////////////////////////////////////////////////////////////////////////
//
// internal data types
//

typedef struct Clock_private {
	Clock		intf;
	int64_t		time_last_change;
	int64_t		time_marked;
	int64_t		last_cycle_count;
} Clock_private;

//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#define	SIGNAL_POOL			clock->signal_pool
#define SIGNAL_COLLECTION	clock->signals
#define SIGNAL_CHIP_ID		-1

#define PRIVATE(clock)	((Clock_private *) (clock))

#ifdef DMS_TIMER_POSIX

static inline void timer_init(void) {
}

static inline int64_t timespec_to_int(struct timespec t) {
	return (int64_t) t.tv_sec * 1000000000 + t.tv_nsec;
}

static inline int64_t timestamp_current(void) {
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return timespec_to_int(ts);
}

#endif // DMS_TIMER_POSIX

#ifdef DMS_TIMER_WIN32

static LARGE_INTEGER qpc_ticks_per_second = { 0 };

static inline void timer_init(void) {
	if (qpc_ticks_per_second.QuadPart == 0 && !QueryPerformanceFrequency(&qpc_ticks_per_second)) {
		// FIXME: error reporting
	}
}

static inline int64_t timestamp_current(void) {
	LARGE_INTEGER count;

	QueryPerformanceCounter(&count);
	return (count.QuadPart * 1000000000) / qpc_ticks_per_second.QuadPart;
}

#endif // DMS_TIMER_WIN32

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Clock *clock_create(int32_t frequency, SignalPool *signal_pool, ClockSignals signals) {
	timer_init();

	Clock_private *priv = (Clock_private *) calloc(1, sizeof(Clock_private));

	Clock *clock = &priv->intf;
	clock->signal_pool = signal_pool;
	clock_set_frequency(clock, frequency);
	clock->real_frequency = 0;
	clock->cycle_count = 0;
	clock->cycle_count = 0;

	priv->time_last_change = timestamp_current();

	memcpy(&clock->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE_BOOL(clock, 1, true);


	return clock;
}

void clock_destroy(Clock *clock) {
	assert(clock);
	free(PRIVATE(clock));
}

void clock_set_frequency(Clock *clock, int32_t frequency) {
	assert(clock);
	assert(frequency > 0);

	clock->conf_frequency = frequency;
	clock->conf_half_period_ns = 1000000000 / (frequency * 2);
}

void clock_mark(Clock *clock) {
	assert(clock);

	int64_t last_marked = PRIVATE(clock)->time_marked;
	PRIVATE(clock)->time_marked = timestamp_current();

	// try to calculate real frequency of the clock
	int64_t time_delta = PRIVATE(clock)->time_marked - last_marked;
	int64_t cycle_delta = clock->cycle_count - PRIVATE(clock)->last_cycle_count;

	if (time_delta > 0 && cycle_delta > 0) {
		clock->real_frequency = (cycle_delta * 1000000000) / time_delta;
	} else {
		clock->real_frequency = clock->conf_frequency;
	}

	PRIVATE(clock)->last_cycle_count = clock->cycle_count;
}

bool clock_is_caught_up(Clock *clock) {
	return PRIVATE(clock)->time_last_change + clock->conf_half_period_ns > PRIVATE(clock)->time_marked;
}

void clock_process(Clock *clock) {
	assert(clock);

	PRIVATE(clock)->time_last_change += clock->conf_half_period_ns;

	bool clock_value = SIGNAL_BOOL(clock);

	clock->cycle_count += clock_value;
	SIGNAL_SET_BOOL(clock, !clock_value);
}

void clock_reset(Clock *clock) {
	assert(clock);
	PRIVATE(clock)->time_last_change = timestamp_current();
}

void clock_refresh(Clock *clock) {
	assert(clock);
	bool clock_value = SIGNAL_BOOL(clock);
	SIGNAL_SET_BOOL(clock, clock_value);
}
