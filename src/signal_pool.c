// signal_pool.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal_pool.h"
#include "signal_line.h"

//
// public functions
//

SignalPool *signal_pool_create(void) {

	SignalPool *pool = (SignalPool *) calloc(1, sizeof(SignalPool));
	sh_new_arena(pool->signal_names);

	signal_pool_set_layer_count(pool, 1);

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

void signal_pool_set_layer_count(SignalPool *pool, uint32_t layer_count) {
	assert(pool);
	assert(layer_count > 0);

	pool->layer_count = layer_count;

	uint64_t layer_mask = (1ull << layer_count) - 1;
	for (int i = 0; i < SIGNAL_BLOCKS; ++i) {
		pool->block_layers[i] = layer_mask;
	}
}

uint64_t signal_pool_cycle(SignalPool *pool) {
	assert(pool);

	// initialize change tracking variables
	memset(pool->signals_changed, 0, sizeof(uint64_t) * SIGNAL_BLOCKS);
	uint64_t dirty_chips = 0;

	// iterate of each block of signals that was changed
	for (uint32_t blocks_touched = pool->blocks_touched; blocks_touched; blocks_touched &= blocks_touched - 1) {
		int blk = bit_lowest_set(blocks_touched);

		// combine the layers
		uint64_t combined_mask = 0;
		uint64_t new_value = 0;

		for (uint64_t layers = pool->block_layers[blk]; layers; layers &= layers - 1) {
			int layer = bit_lowest_set(layers);
			new_value |= pool->signals_next_value[layer][blk] & pool->signals_next_mask[layer][blk];
			combined_mask |= pool->signals_next_mask[layer][blk];
		}

		// apply the defaults
		new_value |= pool->signals_default[blk] & ~combined_mask;

		// determine which signals changed
		pool->signals_changed[blk] = pool->signals_value[blk] ^ new_value;

		// mark dependent chips as dirty
		for (uint64_t changed = pool->signals_changed[blk]; changed; changed &= changed - 1) {
			int32_t signal_idx = bit_lowest_set(changed);
			dirty_chips |= pool->dependent_components[(blk << 6) + signal_idx];
		}

		// apply
		pool->signals_value[blk] = new_value;
	}

	// prepare for next timestep
	pool->blocks_touched = 0;

	return dirty_chips;
}

