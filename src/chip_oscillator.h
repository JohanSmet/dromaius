// chip_oscillator.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Approximation of an oscillator circuit

#ifndef DROMAIUS_CHIP_OSCILLATOR_H
#define DROMAIUS_CHIP_OSCILLATOR_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct OscillatorSignals {
	Signal	clk_out;			// 1-bit clock signal
} OscillatorSignals;

typedef struct Oscillator {

	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	OscillatorSignals	signals;

	// data
	int64_t				half_period_ticks;
	int64_t				tick_next_transition;
} Oscillator;

// functions
Oscillator *oscillator_create(int64_t frequency, SignalPool *pool, OscillatorSignals signals);
void oscillator_register_dependencies(Oscillator *tmr);
void oscillator_destroy(Oscillator *tmr);
void oscillator_process(Oscillator *tmr);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_OSCILLATOR_H
