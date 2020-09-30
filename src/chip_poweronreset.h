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
typedef struct PowerOnResetSignals {
	Signal	trigger_b;
	Signal	reset_b;			// 1-bit reset signal
} PowerOnResetSignals;

typedef struct PowerOnReset {

	CHIP_DECLARE_FUNCTIONS

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
void poweronreset_register_dependencies(PowerOnReset *por);
void poweronreset_destroy(PowerOnReset *por);
void poweronreset_process(PowerOnReset *por);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_POWERONRESET_H
