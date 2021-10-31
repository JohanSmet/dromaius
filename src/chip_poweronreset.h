// chip_poweronreset.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Power-on-reset circuit

#ifndef DROMAIUS_CHIP_POWERONRESET_H
#define DROMAIUS_CHIP_POWERONRESET_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef enum {
	CHIP_POR_TRIGGER_B = CHIP_PIN_01,		// 1-bit trigger signal (input)
	CHIP_POR_RESET_B = CHIP_PIN_02			// 1-bit reset signal (output)
} PowerOnResetSignalAssignment;

#define CHIP_POR_PIN_COUNT 2
typedef Signal PowerOnResetSignals[CHIP_POR_PIN_COUNT];

typedef struct PowerOnReset {

	CHIP_DECLARE_BASE

	// interface
	SignalPool *		signal_pool;
	PowerOnResetSignals	signals;

	// data
	int64_t				duration_ps;
	int64_t				duration_ticks;
	int64_t				next_action;

} PowerOnReset;

// functions
PowerOnReset *poweronreset_create(int64_t reset_duration_ps, struct Simulator *sim, PowerOnResetSignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_POWERONRESET_H
