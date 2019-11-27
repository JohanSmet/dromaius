// clock.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// a configurable oscillator

#ifndef DROMAIUS_CLOCK_H
#define DROMAIUS_CLOCK_H

#include "types.h"

// types
typedef struct Clock {
	// interface
	bool		pin_clock;

	// configuration
	uint32_t	conf_frequency;
	uint64_t	conf_half_period_ns;

	// data
	uint64_t	real_frequency;
	uint64_t	cycle_count;
} Clock;

// functions
Clock *clock_create(uint32_t frequency);
void clock_destroy(Clock *clock);
void clock_set_frequency(Clock *clock, uint32_t frequency);

void clock_process(Clock *clock);
void clock_wait_for_change(Clock *clock);

#endif // DROMAIUS_CLOCK_H