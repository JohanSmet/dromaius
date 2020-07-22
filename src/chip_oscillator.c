// chip_oscillator.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Approximation of an oscillator circuit

#include "chip_oscillator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			tmr->signal_pool
#define SIGNAL_COLLECTION	tmr->signals
#define SIGNAL_CHIP_ID		tmr->id

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Oscillator *oscillator_create(int64_t frequency, SignalPool *pool, OscillatorSignals signals) {
	Oscillator *tmr = (Oscillator *) calloc(1, sizeof(Oscillator));

	tmr->signal_pool = pool;
	CHIP_SET_FUNCTIONS(tmr, oscillator_process, oscillator_destroy, oscillator_register_dependencies);

	memcpy(&tmr->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE_BOOL(clk_out, 1, false);

	tmr->half_period_ticks = 1000000000000l / (frequency * 2 * pool->tick_duration_ps);
	tmr->tick_next_transition = tmr->half_period_ticks;

	return tmr;
}

void oscillator_register_dependencies(Oscillator *tmr) {
	(void) tmr;
}

void oscillator_destroy(Oscillator *tmr) {
	assert(tmr);
	free(tmr);
}

void oscillator_process(Oscillator *tmr) {
	assert(tmr);

	if (tmr->tick_next_transition <= tmr->signal_pool->current_tick) {
		SIGNAL_SET_BOOL(clk_out, !SIGNAL_BOOL(clk_out));
		tmr->tick_next_transition = tmr->signal_pool->current_tick + tmr->half_period_ticks;
	} else {
		SIGNAL_SET_BOOL(clk_out, SIGNAL_BOOL(clk_out));
	}
}

