// chip_poweronreset.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Power-On-Reset circuit

#include "chip_poweronreset.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			por->signal_pool
#define SIGNAL_COLLECTION	por->signals
#define SIGNAL_CHIP_ID		por->id

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

PowerOnReset *poweronreset_create(int64_t duration_ps, SignalPool *pool, PowerOnResetSignals signals) {
	PowerOnReset *por = (PowerOnReset *) calloc(1, sizeof(PowerOnReset));

	por->signal_pool = pool;
	CHIP_SET_FUNCTIONS(por, poweronreset_process, poweronreset_destroy, poweronreset_register_dependencies);

	memcpy(&por->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(reset_b, 1);
	SIGNAL_DEFINE_BOOL(trigger_b, 1, ACTLO_DEASSERT);

	por->duration = signal_pool_interval_to_tick_count(pool, duration_ps);
	por->next_action = por->duration;
	por->schedule_timestamp = por->next_action;

	return por;
}

void poweronreset_register_dependencies(PowerOnReset *por) {
	signal_add_dependency(por->signal_pool, SIGNAL(trigger_b), por->id);
}

void poweronreset_destroy(PowerOnReset *por) {
	assert(por);
	free(por);
}

void poweronreset_process(PowerOnReset *por) {
	assert(por);

	// check for negative transition on the trigger signal
	if (!SIGNAL_BOOL(trigger_b) && signal_changed_last_tick(por->signal_pool, SIGNAL(trigger_b))) {
		por->next_action = por->signal_pool->current_tick + por->duration;
		por->schedule_timestamp = por->next_action;
	}

	bool output = (por->signal_pool->current_tick >= por->next_action);
	SIGNAL_SET_BOOL(reset_b, output);
}

