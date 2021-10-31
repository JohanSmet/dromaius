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
typedef enum {
	CHIP_OSCILLATOR_CLK_OUT = CHIP_PIN_01,			// 1-bit clock signal
} OscillatorSignalAssignment;

#define CHIP_OSCILLATOR_PIN_COUNT 1
typedef Signal OscillatorSignals[CHIP_OSCILLATOR_PIN_COUNT];

typedef struct Oscillator {

	CHIP_DECLARE_BASE

	// interface
	SignalPool *		signal_pool;
	OscillatorSignals	signals;

	// data
	int64_t				frequency;
	int64_t				half_period_ticks;
	int64_t				tick_next_transition;
} Oscillator;

// functions
Oscillator *oscillator_create(int64_t frequency, struct Simulator *sim, OscillatorSignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_OSCILLATOR_H
