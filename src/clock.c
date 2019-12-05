// clock.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// a configurable oscillator

#include "clock.h"

#include <assert.h>
#include <stdlib.h>

#if defined(PLATFORM_LINUX) || defined(PLATFORM_EMSCRIPTEN)
	#include <time.h>
#endif // PLATFORM_LINUX or PLATFORM_EMSCRIPTEN

//////////////////////////////////////////////////////////////////////////////
//
// internal data types
//

typedef struct Clock_private {
	Clock		intf;
	int64_t		time_next_change;
	int64_t		time_last_change;
} Clock_private;

//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#define PRIVATE(clock)	((Clock_private *) (clock))

#if defined(PLATFORM_LINUX) || defined(PLATFORM_EMSCRIPTEN)

static inline int64_t timespec_to_int(struct timespec t) {
	return t.tv_sec * 1000000000 + t.tv_nsec;
}

static inline int64_t timestamp_current(void) {
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return timespec_to_int(ts);
}

#endif // PLATFORM_LINUX or PLATFORM_EMSCRIPTEN

static inline void process_update(Clock *clock, int64_t cur_time) {
	// FIXME: smoothing would be nice (probably)
	clock->real_frequency = 1e9 / ((cur_time - PRIVATE(clock)->time_last_change) * 2);

	PRIVATE(clock)->time_last_change = cur_time;
	PRIVATE(clock)->time_next_change = cur_time + clock->conf_half_period_ns;
	clock->cycle_count += clock->pin_clock;
	clock->pin_clock = !clock->pin_clock;
}

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Clock *clock_create(uint32_t frequency) {
	Clock_private *clock = (Clock_private *) malloc(sizeof(Clock_private));
	
	clock->intf.pin_clock = true;
	clock_set_frequency(&clock->intf, frequency);
	clock->intf.real_frequency = 0;
	clock->intf.cycle_count = 0;
	
	clock->time_last_change = timestamp_current();
	clock->time_next_change = clock->time_last_change + clock->intf.conf_half_period_ns;

	return (Clock *) clock;
}

void clock_destroy(Clock *clock) {
	assert(clock);
	free(PRIVATE(clock));
}

void clock_set_frequency(Clock *clock, uint32_t frequency) {
	assert(clock);

	clock->conf_frequency = frequency;
	clock->conf_half_period_ns = (frequency != 0) ? 1e9 / (frequency * 2) : 0;
	PRIVATE(clock)->time_next_change = PRIVATE(clock)->time_last_change + clock->conf_half_period_ns;
}

void clock_process(Clock *clock) {
	assert(clock);

	// check if it is time to toggle the output
	int64_t cur_time = timestamp_current();

	if (cur_time >= PRIVATE(clock)->time_next_change) {
		process_update(clock, cur_time);
	}
}

void clock_wait_for_change(Clock *clock) {
	assert(clock);

	// conf_frequency == 0 means always toggle
	if (clock->conf_frequency == 0) {
		clock->pin_clock = !clock->pin_clock;
		clock->cycle_count += clock->pin_clock;
		return;
	}

	// check if it is time to toggle the output
	int64_t cur_time = timestamp_current();

	while (cur_time < PRIVATE(clock)->time_next_change) {
		// FIXME: try to sleep?
		cur_time = timestamp_current();
	}

	process_update(clock, cur_time);
}

