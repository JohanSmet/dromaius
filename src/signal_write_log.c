// signal_change_queue.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// track changes to signals during a simulation timestep

#include "signal_write_log.h"

#include <assert.h>
#include <stdlib.h>

SignalWriteLog *signal_write_log_create(size_t max_writes_per_step, size_t num_queues) {
	size_t queue_size = sizeof(SignalWriteQueue) + (sizeof(SignalQueueEntry) * max_writes_per_step);
	size_t alloc_size = sizeof(SignalWriteLog) + (queue_size * num_queues) + (sizeof(SignalWriteQueue *) * num_queues);

	SignalWriteLog *log = calloc(1, alloc_size);
	log->num_queues = num_queues;
	uint8_t *queue_ptr = (uint8_t *) log + sizeof(SignalWriteLog) + (sizeof(SignalWriteQueue *) * num_queues);

	for (size_t q = 0; q < num_queues; ++q) {
		log->queues[q] = (SignalWriteQueue *) queue_ptr;
		queue_ptr += queue_size;
	}

	return log;
}

void signal_write_log_destroy(SignalWriteLog *write_log) {
	assert(write_log);
	free(write_log);
}

