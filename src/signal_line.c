// signal_line.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal_line.h"

#ifdef DMS_SIGNAL_TRACING
	#include "signal_dumptrace.h"
#endif

#include <stb/stb_ds.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

Signal signal_create(SignalPool *pool) {
	assert(pool);
	assert(pool->signals_count < SIGNAL_MAX_DEFINED);

	Signal result = pool->signals_count++;

	arrpush(pool->signals_name, NULL);
	arrpush(pool->signals_layers, 0ull);
	arrpush(pool->dependent_components, 0);

	return result;
}

void signal_set_name(SignalPool *pool, Signal signal, const char *name) {
	assert(pool);
	assert(name);

	shput(pool->signal_names, name, signal);
	pool->signals_name[signal] = pool->signal_names[shlenu(pool->signal_names) - 1].key;
}

const char *signal_get_name(SignalPool *pool, Signal signal) {
	assert(pool);

	const char *name = pool->signals_name[signal];
	return (name == NULL) ? "" : name;
}

void signal_add_dependency(SignalPool *pool, Signal signal, int32_t chip_id) {
	assert(pool);
	assert(chip_id >= 0 && chip_id < 64);

	uint64_t dep_mask = 1ull << chip_id;
	pool->dependent_components[signal] |= dep_mask;
}

void signal_default(SignalPool *pool, Signal signal, bool value) {
	assert(pool);

	uint32_t signal_block = (signal & 0xffffffc0) >> 6;
	uint64_t signal_flag = 1ull << (signal & 0x3f);

	FLAG_SET_CLEAR_U64(pool->signals_default[signal_block], signal_flag, value);
	FLAG_SET_CLEAR_U64(pool->signals_value[signal_block], signal_flag, value);
}

Signal signal_by_name(SignalPool *pool, const char *name) {
	return shget(pool->signal_names, name);
}

bool signal_read_next(SignalPool *pool, Signal signal) {
	assert(pool);

	uint32_t signal_block = (signal & 0xffffffc0) >> 6;
	uint64_t signal_flag = 1ull << (signal & 0x3f);

	// first layer
	uint64_t value = (pool->signals_next_value[0][signal_block] & pool->signals_next_mask[0][signal_block]) & signal_flag;
	uint64_t combined_mask = pool->signals_next_mask[0][signal_block] & signal_flag;

	// next layers
	for (uint32_t layer = 1; layer < pool->layer_count; ++layer) {
		value |= (pool->signals_next_value[layer][signal_block] & pool->signals_next_mask[layer][signal_block]) & signal_flag;
		combined_mask |= pool->signals_next_mask[layer][signal_block] & signal_flag;
	}

	// default
	value |= (pool->signals_default[signal_block] & ~combined_mask) & signal_flag;

	return (value & signal_flag) == signal_flag;
}

void signal_group_set_name(SignalPool *pool, SignalGroup sg, const char *group_name, const char *signal_name, uint32_t start_idx) {
	assert(pool);
	assert(sg);
	assert(group_name);
	assert(signal_name);

	// TODO: store group name
	(void) group_name;

	// name individual signals
	char sub_name[MAX_SIGNAL_NAME];
	for (uint32_t i = 0; i < signal_group_size(sg); ++i) {
		snprintf(sub_name, MAX_SIGNAL_NAME, signal_name, i + start_idx);
		shput(pool->signal_names, sub_name, sg[i]);
		pool->signals_name[sg[i]] = pool->signal_names[shlenu(pool->signal_names) - 1].key;
	}
}
