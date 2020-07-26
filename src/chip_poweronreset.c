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

	por->por_ticks = signal_pool_interval_to_tick_count(pool, duration_ps);
	por->schedule_timestamp = por->por_ticks;

	return por;
}

void poweronreset_register_dependencies(PowerOnReset *por) {
	(void) por;
}

void poweronreset_destroy(PowerOnReset *por) {
	assert(por);
	free(por);
}

void poweronreset_process(PowerOnReset *por) {
	assert(por);

	bool output = (por->signal_pool->current_tick >= por->por_ticks);
	SIGNAL_SET_BOOL(reset_b, output);
}

