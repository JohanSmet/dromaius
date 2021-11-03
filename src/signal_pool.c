// signal_pool.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal_pool.h"
#include "signal_line.h"

//
// public functions
//

SignalPool *signal_pool_create(void) {

	SignalPool *pool = (SignalPool *) calloc(1, sizeof(SignalPool));
	sh_new_arena(pool->signal_names);

	pool->layer_count = 1;

	// NULL-signal: to detect uninitialized signals
	signal_create(pool);

	return pool;
}

void signal_pool_destroy(SignalPool *pool) {
	assert(pool);

	arrfree(pool->signals_name);
	arrfree(pool->dependent_components);

	free(pool);
}

uint64_t signal_pool_cycle(SignalPool *pool) {
	assert(pool);

	uint64_t combined_mask[SIGNAL_BLOCKS];
	uint64_t old_value[SIGNAL_BLOCKS];

	// copy over the first layer
	memcpy(combined_mask, pool->signals_next_mask[0], sizeof(uint64_t) * SIGNAL_BLOCKS);
	for (int blk = 0; blk < SIGNAL_BLOCKS; ++blk) {
		old_value[blk] = pool->signals_value[blk];
		pool->signals_value[blk] = pool->signals_next_value[0][blk] & pool->signals_next_mask[0][blk];
	}

	// merge in the other layers
	for (uint32_t layer = 1; layer < pool->layer_count; ++layer) {
		for (int blk = 0; blk < SIGNAL_BLOCKS; ++blk) {
			pool->signals_value[blk] |= pool->signals_next_value[layer][blk] & pool->signals_next_mask[layer][blk];
			combined_mask[blk] |= pool->signals_next_mask[layer][blk];
		}
	}

	// apply the defaults
	for (int blk = 0; blk < SIGNAL_BLOCKS; ++blk) {
		pool->signals_value[blk] |= pool->signals_default[blk] & ~combined_mask[blk];
	}

	// determine which signals changed
	for (int blk = 0; blk < SIGNAL_BLOCKS; ++blk) {
		pool->signals_changed[blk] = old_value[blk] ^ pool->signals_value[blk];
	}

	// determine which chips are dirty now
	uint64_t dirty_chips = 0;

	for (int blk = 0; blk < SIGNAL_BLOCKS; ++blk) {
		uint64_t changed = pool->signals_changed[blk];

		while (changed > 0) {

			// find changed signal with lowest index
			int32_t signal_idx = bit_lowest_set(changed);

			dirty_chips |= pool->dependent_components[(blk << 6) + signal_idx];

			// clear lowest set bit
			changed &= changed - 1;
		}
	}

	return dirty_chips;
}

