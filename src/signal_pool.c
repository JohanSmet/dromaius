// signal_pool.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal_pool.h"
#include "signal_line.h"

#ifdef DMS_SIGNAL_STATISTICS
#include <stdio.h>
#endif

//
// private (helper) functions
//

static inline uint64_t signal_pool_process_high_impedance__internal(SignalPool *pool) {
	assert(pool);

	uint64_t rerun_chips = 0;

	for (size_t q = 0, nq = pool->signals_highz_log->num_queues; q < nq; ++q) {
		SignalWriteQueue *queue = pool->signals_highz_log->queues[q];

		for (size_t i = 0, n = queue->count; i < n; ++i) {
			const Signal s = queue->writes[i].signal;
			const uint64_t w_mask = queue->writes[i].writer_mask;

			// clear the corresponding bit in the writers mask
			pool->signals_writers[s] &= ~w_mask;

			if (pool->signals_writers[s] != 0) {
				rerun_chips |= pool->signals_writers[s];
			} else {
				signal_write_log_add(pool->signals_write_log, 0, s, pool->signals_default[s], 0);
			}
		}

		queue->count = 0;
	}

	return rerun_chips;
}

//
// public functions
//

SignalPool *signal_pool_create(size_t max_concurrent_writes) {

	SignalPool *pool = (SignalPool *) calloc(1, sizeof(SignalPool));
	sh_new_arena(pool->signal_names);

	// NULL-signal: to detect uninitialized signals
	signal_create(pool);

	// create queues
	pool->signals_write_log = signal_write_log_create(max_concurrent_writes, 2);
	pool->signals_highz_log = signal_write_log_create(max_concurrent_writes, 1);

	return pool;
}

void signal_pool_destroy(SignalPool *pool) {
	assert(pool);

	arrfree(pool->signals_value);
	arrfree(pool->signals_writers);
	arrfree(pool->signals_next_value);
	arrfree(pool->signals_next_writers);
	arrfree(pool->signals_default);
	arrfree(pool->signals_name);
	arrfree(pool->signals_changed);
	arrfree(pool->dependent_components);

	signal_write_log_destroy(pool->signals_write_log);
	signal_write_log_destroy(pool->signals_highz_log);

	free(pool);
}

uint64_t signal_pool_process_high_impedance(SignalPool *pool) {
	return signal_pool_process_high_impedance__internal(pool);
}

uint64_t signal_pool_cycle(SignalPool *pool) {

	signal_pool_process_high_impedance__internal(pool);

#ifdef DMS_SIGNAL_STATISTICS
	size_t stat_total_writes   = 0;
	size_t stat_signal_changes = 0;
#endif // DMS_SIGNAL_STATISTICS

	// handle writes
	memset(pool->signals_changed, false, arrlenu(pool->signals_changed));
	uint64_t dirty_chips = 0;

	for (size_t q = 0, nq = pool->signals_write_log->num_queues; q < nq; ++q) {
		SignalWriteQueue *queue = pool->signals_write_log->queues[q];

		for (size_t i = 0, n = queue->count; i < n; ++i) {
			const SignalQueueEntry *sw = &queue->writes[i];
			pool->signals_changed[sw->signal] = pool->signals_value[sw->signal] != sw->new_value;
		}
	}

	for (size_t q = 0, nq = pool->signals_write_log->num_queues; q < nq; ++q) {
		SignalWriteQueue *queue = pool->signals_write_log->queues[q];

		for (size_t i = 0, n = queue->count; i < n; ++i) {
			const SignalQueueEntry *sw = &queue->writes[i];

			pool->signals_writers[sw->signal] |= sw->writer_mask;
			pool->signals_value[sw->signal] = sw->new_value;

			const uint64_t mask = (uint64_t) (!pool->signals_changed[sw->signal] - 1);
			dirty_chips |= mask & pool->dependent_components[sw->signal];

			#ifdef DMS_SIGNAL_STATISTICS
				stat_signal_changes += pool->signals_changed[sw->signal];
			#endif // DMS_SIGNAL_STATISTICS
		}

		#ifdef DMS_SIGNAL_STATISTICS
			stat_total_writes += queue->count;
		#endif // DMS_SIGNAL_STATISTICS

		queue->count = 0;
	}

	#ifdef DMS_SIGNAL_STATISTICS
		printf("%ld %ld\n", stat_signal_changes, stat_total_writes);
	#endif // DMS_SIGNAL_STATISTICS

	pool->swq_snapshot = 0;
	pool->cycle_count += 1;

	return dirty_chips;
}

