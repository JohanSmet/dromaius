// signal_pool.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_SIGNAL_POOL_H
#define DROMAIUS_SIGNAL_POOL_H

#include "signal_types.h"
#include "signal_write_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct SignalNameMap {
	const char *key;
	Signal		value;
} SignalNameMap;

typedef struct SignalPool {
	bool *			signals_value;				// current value of the signal
	uint64_t *		signals_writers;			// id's of the chips that are actively writing to the signal (bit-mask)
	bool *			signals_changed;			// did the signal change since the last cycle

	bool *			signals_default;			// value of the signal when nobody is writing to it (pull-up or down)
	uint64_t *		dependent_components;		// id's of the chips that depend on this signal

	const char **	signals_name;				// names of the signal (id -> name)
	SignalNameMap *	signal_names;				// hashmap name -> signal

	SignalWriteLog *signals_write_log;			// queue of pending writes to the signals
	SignalWriteLog *signals_highz_log;			// queue of signals that have had an active writer go away

	// variables specifically to make read_next work (normally only used in unit-tests)
	bool *			signals_next_value;
	uint64_t *		signals_next_writers;
	size_t			swq_snapshot;
	size_t			cycle_count;
	size_t			read_next_cycle;

#ifdef DMS_SIGNAL_TRACING
	struct SignalTrace *trace;
#endif
} SignalPool;

// functions - signal pool
SignalPool *signal_pool_create(size_t max_concurrent_writes);
void signal_pool_destroy(SignalPool *pool);

uint64_t signal_pool_process_high_impedance(SignalPool *pool);
uint64_t signal_pool_cycle(SignalPool *pool);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIGNAL_POOL_H
