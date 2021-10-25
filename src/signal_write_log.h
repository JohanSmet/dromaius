// signal_change_queue.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// track changes to signals during a simulation timestep

#ifndef DROMAIUS_SIGNAL_CHANGE_QUEUE_H
#define DROMAIUS_SIGNAL_CHANGE_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "signal_types.h"

// types
typedef struct {
	Signal				signal;					// signal to write to
	uint64_t			writer_mask;			// id-mask of the chip writing to the signal
	bool				new_value;				// value to write
} SignalQueueEntry;

typedef struct SignalWriteQueue {
	size_t				count;
	SignalQueueEntry	writes[];
} SignalWriteQueue;

typedef struct SignalWriteLog {
	size_t				num_queues;
	SignalWriteQueue *	queues[];
} SignalWriteLog;

// functions
SignalWriteLog *signal_write_log_create(size_t max_writes_per_step, size_t num_queues);
void signal_write_log_destroy(SignalWriteLog *write_log);

static inline void signal_write_log_add(SignalWriteLog *write_log, size_t queue, Signal signal, bool value, uint64_t w_mask) {
	SignalWriteQueue *w_queue = write_log->queues[queue];
	SignalQueueEntry *entry = &w_queue->writes[w_queue->count++];
	entry->signal		= signal;
	entry->writer_mask	= w_mask;
	entry->new_value	= value;
}

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIGNAL_CHANGE_QUEUE_H
