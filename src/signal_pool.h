// signal_pool.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_SIGNAL_POOL_H
#define DROMAIUS_SIGNAL_POOL_H

#include "signal_types.h"
#include <assert.h>
#include <string.h>
#include <stb/stb_ds.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIGNAL_BLOCK_SIZE	64
#define SIGNAL_BLOCKS		6
#define SIGNAL_MAX_DEFINED	(SIGNAL_BLOCK_SIZE * SIGNAL_BLOCKS)
#define SIGNAL_LAYERS		16

// types
typedef struct SignalNameMap {
	char *key;
	Signal		value;
} SignalNameMap;

typedef struct SignalPool {

	uint32_t		signals_count;										// the number of defined signals
	uint32_t		layer_count;										// the number of signal layers that are used

	uint64_t		signals_value[SIGNAL_BLOCKS];						// current value of the signals (read-only)
	uint64_t		signals_changed[SIGNAL_BLOCKS];						// did the signal change in the previous timestep

	uint32_t		blocks_touched;										// bitmask to track which blocks have to be processed
	uint8_t			block_layer_count[SIGNAL_BLOCKS];					// the maximum number of layers used by signals in this block

	uint64_t		signals_next_value[SIGNAL_LAYERS][SIGNAL_BLOCKS];	// value of the signals after this timestep
	uint64_t		signals_next_mask[SIGNAL_LAYERS][SIGNAL_BLOCKS];	// write mask of the signals
	uint64_t		signals_default[SIGNAL_BLOCKS];						// default value of signals if not explicitly written to

	int32_t 		signals_layer_component[SIGNAL_MAX_DEFINED][SIGNAL_LAYERS];				// which components is used on each layer per signal
	uint64_t *		dependent_components;								// ids of the chips that depend on each signal

	char **			signals_name;										// names of the signal (id -> name)
	SignalNameMap	*signal_names;										// hashmap name -> signal

} SignalPool;

// functions - signal pool
SignalPool *signal_pool_create(void);
void signal_pool_destroy(SignalPool *pool);
void signal_pool_set_layer_count(SignalPool *pool, uint8_t layer_count);

uint64_t signal_pool_cycle(SignalPool *pool);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIGNAL_POOL_H
