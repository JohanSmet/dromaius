// chip_poweronreset.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Power-On-Reset circuit

#define SIGNAL_ARRAY_STYLE

#include "chip_poweronreset.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_PREFIX		CHIP_POR_
#define SIGNAL_OWNER		por

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void poweronreset_register_dependencies(PowerOnReset *por);
static void poweronreset_destroy(PowerOnReset *por);
static void poweronreset_process(PowerOnReset *por);

PowerOnReset *poweronreset_create(int64_t duration_ps, Simulator *sim, PowerOnResetSignals signals) {
	PowerOnReset *por = (PowerOnReset *) calloc(1, sizeof(PowerOnReset));

	por->simulator = sim;
	por->signal_pool = sim->signal_pool;
	CHIP_SET_FUNCTIONS(por, poweronreset_process, poweronreset_destroy, poweronreset_register_dependencies);

	memcpy(por->signals, signals, sizeof(PowerOnResetSignals));
	SIGNAL_DEFINE(CHIP_POR_RESET_B);
	SIGNAL_DEFINE_DEFAULT(CHIP_POR_TRIGGER_B, ACTLO_DEASSERT);

	por->duration_ps = duration_ps;
	por->duration_ticks = simulator_interval_to_tick_count(por->simulator, por->duration_ps);
	por->next_action = por->simulator->current_tick + por->duration_ticks;
	por->schedule_timestamp = por->next_action;

	return por;
}

static void poweronreset_register_dependencies(PowerOnReset *por) {
	SIGNAL_DEPENDENCY(TRIGGER_B);
}

static void poweronreset_destroy(PowerOnReset *por) {
	assert(por);
	free(por);
}

static void poweronreset_process(PowerOnReset *por) {
	assert(por);

	// check for negative transition on the trigger signal
	if (!SIGNAL_READ(TRIGGER_B) && SIGNAL_CHANGED(TRIGGER_B)) {
		por->next_action = por->simulator->current_tick + por->duration_ticks;
		por->schedule_timestamp = por->next_action;
	}

	bool output = (por->simulator->current_tick >= por->next_action);
	SIGNAL_WRITE(RESET_B, output);
}

