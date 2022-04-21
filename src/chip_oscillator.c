// chip_oscillator.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Approximation of an oscillator circuit

#include "chip_oscillator.h"
#include "simulator.h"

#include "crt.h"

#define SIGNAL_PREFIX		CHIP_OSCILLATOR_
#define SIGNAL_OWNER		tmr

static uint8_t ChipOscillator_PinTypes[CHIP_OSCILLATOR_PIN_COUNT] = {
	[CHIP_OSCILLATOR_CLK_OUT] = CHIP_PIN_OUTPUT
};

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void oscillator_destroy(Oscillator *tmr);
static void oscillator_process(Oscillator *tmr);

Oscillator *oscillator_create(int64_t frequency, Simulator *sim, OscillatorSignals signals) {
	Oscillator *tmr = (Oscillator *) dms_calloc(1, sizeof(Oscillator));

	CHIP_SET_FUNCTIONS(tmr, oscillator_process, oscillator_destroy);
	CHIP_SET_VARIABLES(tmr, sim, tmr->signals, ChipOscillator_PinTypes, CHIP_OSCILLATOR_PIN_COUNT);

	tmr->signal_pool = sim->signal_pool;

	dms_memcpy(tmr->signals, signals, sizeof(OscillatorSignals));
	SIGNAL_DEFINE_DEFAULT(CLK_OUT, false);

	tmr->frequency = frequency;
	tmr->half_period_ticks = 1000000000000l / (tmr->frequency * 2 * tmr->simulator->tick_duration_ps);
	tmr->tick_next_transition = tmr->half_period_ticks;
	tmr->schedule_timestamp = tmr->tick_next_transition;

	return tmr;
}

static void oscillator_destroy(Oscillator *tmr) {
	assert(tmr);
	dms_free(tmr);
}

static void oscillator_process(Oscillator *tmr) {
	assert(tmr);

	if (tmr->tick_next_transition <= tmr->simulator->current_tick) {
		SIGNAL_WRITE(CLK_OUT, !SIGNAL_READ(CLK_OUT));
		tmr->tick_next_transition = tmr->simulator->current_tick + tmr->half_period_ticks;
		tmr->schedule_timestamp = tmr->tick_next_transition;
	}
}

