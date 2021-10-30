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

	Signal result = (uint32_t) arrlenu(pool->signals_value);

	arrpush(pool->signals_changed, false);
	arrpush(pool->signals_value, false);
	arrpush(pool->signals_next_value, 0);
	arrpush(pool->signals_default, false);
	arrpush(pool->signals_name, NULL);
	arrpush(pool->signals_writers, 0);
	arrpush(pool->signals_next_writers, 0);
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

	pool->signals_default[signal] = value;
	pool->signals_value[signal] = value;
}

Signal signal_by_name(SignalPool *pool, const char *name) {
	return shget(pool->signal_names, name);
}

bool signal_read_next(SignalPool *pool, Signal signal) {
	assert(pool);

	if (pool->read_next_cycle != pool->cycle_count ||
		pool->swq_snapshot != pool->signals_write_queue[1].size + pool->signals_highz_queue.size) {

		memcpy(pool->signals_next_value, pool->signals_value, sizeof(*pool->signals_value) * arrlenu(pool->signals_value));
		memcpy(pool->signals_next_writers, pool->signals_writers, sizeof(*pool->signals_writers) * arrlenu(pool->signals_writers));

		for (size_t i = 0, n = pool->signals_highz_queue.size; i < n; ++i) {
			const Signal s = pool->signals_highz_queue.queue[i].signal;
			const uint64_t w_mask = pool->signals_highz_queue.queue[i].writer_mask;

			// clear the corresponding bit in the writers mask
			pool->signals_next_writers[s] &= ~w_mask;

			if (pool->signals_next_writers[s] == 0) {
				pool->signals_next_value[s] =  pool->signals_default[s];
			}
		}

		for (size_t i = 0, n = pool->signals_write_queue[1].size; i < n; ++i) {
			SignalQueueEntry *sw = &pool->signals_write_queue[1].queue[i];
			pool->signals_next_value[sw->signal] = sw->new_value;
		}

		pool->swq_snapshot = pool->signals_write_queue[1].size + pool->signals_highz_queue.size;
		pool->read_next_cycle = pool->cycle_count;
	}

	return pool->signals_next_value[signal];
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
