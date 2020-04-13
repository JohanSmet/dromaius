// clock.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// a configurable oscillator

#ifndef DROMAIUS_CLOCK_H
#define DROMAIUS_CLOCK_H

#include "types.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct ClockSignals {
	Signal	clock;
} ClockSignals;

typedef struct Clock {
	// interface
	SignalPool *	signal_pool;
	ClockSignals	signals;

	// configuration
	int32_t		conf_frequency;
	int64_t		conf_half_period_ns;

	// data
	int64_t 	real_frequency;
	int64_t		cycle_count;
} Clock;

// functions
Clock *clock_create(int32_t frequency, SignalPool *signal_pool, ClockSignals signals);
void clock_destroy(Clock *clock);
void clock_set_frequency(Clock *clock, int32_t frequency);

void clock_mark(Clock *clock);
bool clock_is_caught_up(Clock *clock);
void clock_process(Clock *clock);
void clock_reset(Clock *clock);
void clock_refresh(Clock *clock);
void clock_wait_for_change(Clock *clock);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CLOCK_H
