// chip_poweronreset.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Power-On-Reset circuit

#include "chip_poweronreset.h"
#include "simulator.h"
#include "crt.h"

#define SIGNAL_PREFIX		CHIP_POR_
#define SIGNAL_OWNER		por

static uint8_t ChipPor_PinTypes[CHIP_POR_PIN_COUNT] = {
	[CHIP_POR_TRIGGER_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_POR_RESET_B  ] = CHIP_PIN_OUTPUT
};

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void poweronreset_destroy(PowerOnReset *por);
static void poweronreset_process(PowerOnReset *por);

PowerOnReset *poweronreset_create(int64_t duration_ps, Simulator *sim, PowerOnResetSignals signals) {
	PowerOnReset *por = (PowerOnReset *) dms_calloc(1, sizeof(PowerOnReset));

	CHIP_SET_FUNCTIONS(por, poweronreset_process, poweronreset_destroy);
	CHIP_SET_VARIABLES(por, sim, por->signals, ChipPor_PinTypes, CHIP_POR_PIN_COUNT);

	por->signal_pool = sim->signal_pool;
	dms_memcpy(por->signals, signals, sizeof(PowerOnResetSignals));
	SIGNAL_DEFINE(RESET_B);
	SIGNAL_DEFINE_DEFAULT(TRIGGER_B, ACTLO_DEASSERT);

	por->duration_ps = duration_ps;
	por->duration_ticks = simulator_interval_to_tick_count(por->simulator, por->duration_ps);
	por->next_action = por->simulator->current_tick + por->duration_ticks;
	por->schedule_timestamp = por->next_action;

	return por;
}

static void poweronreset_destroy(PowerOnReset *por) {
	assert(por);
	dms_free(por);
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

