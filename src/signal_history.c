// signal_history.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal_history.h"
#include "signal_pool.h"
#include "signal_line.h"

#include "sys/threads.h"

#include <stb/stb_ds.h>
#include <stdatomic.h>

typedef struct HistoryIncoming {
	int64_t		time;
	uint64_t	signals_value[SIGNAL_BLOCKS];
	uint64_t	signals_changed[SIGNAL_BLOCKS];
} HistoryIncoming;

typedef struct SignalHistory_private {
	SignalHistory	public;

	atomic_uint		next_in;
	atomic_uint		first_out;

	atomic_bool		thread_stop_request;
	thread_t		thread;

	mutex_t			mtx_work;
	cond_t			cnd_work;

	mutex_t			mtx_ui_access;
} SignalHistory_private;

#define PRIVATE(cpu)	((SignalHistory_private *) (cpu))

static inline unsigned int next_index(SignalHistory *history, unsigned int idx) {
	return (idx + 1) % history->incoming_count;
}

static void prepare_diagram_data(SignalHistoryDiagramData *data) {

	if (data->signal_start_offsets) {
		stbds_header(data->signal_start_offsets)->length = 0;
	}

	if (data->samples_time) {
		stbds_header(data->samples_time)->length = 0;
	}

	if (data->samples_value) {
		stbds_header(data->samples_value)->length = 0;
	}
}

static int signal_history_processing_thread(SignalHistory *history) {

	while (!PRIVATE(history)->thread_stop_request) {

		mutex_lock(&PRIVATE(history)->mtx_work);

		// wait for work
		while (!PRIVATE(history)->thread_stop_request && PRIVATE(history)->first_out == atomic_load(&PRIVATE(history)->next_in)) {
			cond_wait(&PRIVATE(history)->cnd_work, &PRIVATE(history)->mtx_work);
		}

		// execute all work
		while (signal_history_process_incoming_single(history));

		mutex_unlock(&PRIVATE(history)->mtx_work);
	}

	return 0;
}

//
// interface
//

SignalHistory *signal_history_create(size_t incoming_count, size_t signal_count, size_t sample_count) {
	SignalHistory_private *priv = (SignalHistory_private *) calloc(1, sizeof(SignalHistory_private));
	SignalHistory *history = &priv->public;

	history->incoming_count = (unsigned int) incoming_count;
	history->incoming = (HistoryIncoming *) malloc(sizeof(HistoryIncoming) * incoming_count);

	history->signal_count = signal_count;
	history->sample_count = sample_count;
	history->signal_samples_base = (size_t *) calloc(3 * signal_count, sizeof(size_t));
	history->signal_samples_head = history->signal_samples_base + signal_count;
	history->signal_samples_tail = history->signal_samples_head + signal_count;

	history->samples_time = (int64_t *) malloc(sizeof(int64_t) * signal_count * sample_count);
	history->samples_value = (bool *) malloc(sizeof(bool) * signal_count * sample_count);

	for (size_t si = 0; si < signal_count; ++si) {
		history->signal_samples_base[si] = si * sample_count;
		history->signal_samples_head[si] = (size_t) -1;
		history->signal_samples_tail[si] = (size_t) -1;
	}

	// private variables
	mutex_init_plain(&priv->mtx_work);
	cond_init(&priv->cnd_work);
	mutex_init_plain(&priv->mtx_ui_access);

	return history;
}

void signal_history_destroy(SignalHistory *history) {
	assert(history);
	free(history->incoming);
	free(history->signal_samples_base);
	free(history->samples_time);
	free(history->samples_value);
	free(history);
}

void signal_history_process_start(SignalHistory *history) {
	assert(history);

	PRIVATE(history)->thread_stop_request = false;
	history->capture_active = true;
	thread_create_joinable(&PRIVATE(history)->thread, (thread_func_t) signal_history_processing_thread, PRIVATE(history));
}

void signal_history_process_stop(SignalHistory *history) {
	assert(history);

	mutex_lock(&PRIVATE(history)->mtx_work);
	PRIVATE(history)->thread_stop_request = true;
	mutex_unlock(&PRIVATE(history)->mtx_work);

	if (history->capture_active) {
		cond_signal(&PRIVATE(history)->cnd_work);
		history->capture_active = false;

		int thread_res;
		thread_join(PRIVATE(history)->thread, &thread_res);
	}

	// reset
	PRIVATE(history)->next_in = 0;
	PRIVATE(history)->first_out = 0;
	for (size_t si = 0; si < history->signal_count; ++si) {
		history->signal_samples_head[si] = (size_t) -1;
		history->signal_samples_tail[si] = (size_t) -1;
	}
}

#include <stdio.h>

bool signal_history_add(SignalHistory *history, int64_t time, uint64_t *signals_value, uint64_t *signals_changed, bool block) {
	// runs on the simulator thread
	assert(history);

	// don't let next_in index 'catch up' with first_out index
	while (next_index(history, PRIVATE(history)->next_in) == atomic_load(&PRIVATE(history)->first_out)) {
		if (!block) {
			return false;
		}
		// spin-lock: first_out is updated in another thread
		cond_signal(&PRIVATE(history)->cnd_work);
	}

	// copy values
	HistoryIncoming *in = &history->incoming[PRIVATE(history)->next_in];
	in->time = time;
	memcpy(in->signals_value, signals_value, sizeof(uint64_t) * SIGNAL_BLOCKS);
	memcpy(in->signals_changed, signals_changed, sizeof(uint64_t) * SIGNAL_BLOCKS);

	// move pointer along -- there's only one thread writing to next_in
	atomic_store(&PRIVATE(history)->next_in, next_index(history, PRIVATE(history)->next_in));

	// kick worker thread awake if necessary
	cond_signal(&PRIVATE(history)->cnd_work);

	return true;
}

void signal_history_diagram_data(struct SignalHistory *history, SignalHistoryDiagramData *diagram_data) {
	assert(history);
	assert(diagram_data);

	// clear any data that's already in there
	prepare_diagram_data(diagram_data);

	mutex_lock(&PRIVATE(history)->mtx_ui_access);

	// iterate signals
	for (size_t idx = 0; idx < arrlenu(diagram_data->signals); ++idx) {

		size_t si = signal_array_subscript(diagram_data->signals[idx]);

		// save start of data for this signal
		arrpush(diagram_data->signal_start_offsets, arrlenu(diagram_data->samples_time));

		// iterate over store changes of signal data
		size_t base = history->signal_samples_base[si];
		size_t cur = history->signal_samples_head[si];
		size_t end = history->signal_samples_tail[si];

		if (cur > history->sample_count) {
			// no samples for signal
			continue;
		}

		for (;;) {

			size_t sample = base + cur;

			if (history->samples_time[sample] < diagram_data->time_end) {
				arrpush(diagram_data->samples_time, history->samples_time[sample]);
				arrpush(diagram_data->samples_value, history->samples_value[sample]);
			}

			if (history->samples_time[sample] < diagram_data->time_begin || cur == end)  {
				break;
			}

			cur = (cur - 1 + history->sample_count) % history->sample_count;
		}
	}

	arrpush(diagram_data->signal_start_offsets, arrlenu(diagram_data->samples_time));

	mutex_unlock(&PRIVATE(history)->mtx_ui_access);
}

void signal_history_diagram_release(SignalHistoryDiagramData *diagram_data) {
	assert(diagram_data);
	arrfree(diagram_data->signals);
	arrfree(diagram_data->signal_start_offsets);
	arrfree(diagram_data->samples_time);
	arrfree(diagram_data->samples_value);
}

void signal_history_store_data(SignalHistory *history, size_t signal, int64_t time, bool value) {
	assert(history);
	assert(signal < history->signal_count);

	// store
	size_t idx = history->signal_samples_base[signal] + ((history->signal_samples_head[signal] + 1) % history->sample_count);
	history->samples_time[idx] = time;
	history->samples_value[idx] = value;

	// move pointers
	history->signal_samples_head[signal] = (history->signal_samples_head[signal] + 1) % history->sample_count;
	if (history->signal_samples_head[signal] == history->signal_samples_tail[signal] || history->signal_samples_tail[signal] == (size_t) -1) {
		history->signal_samples_tail[signal] = (history->signal_samples_tail[signal] + 1) % history->sample_count;
	}
}

bool signal_history_process_incoming_single(SignalHistory *history) {
	// runs on the dedicated history thread
	assert(history);

	// check if there is work to do
	if (PRIVATE(history)->first_out == atomic_load(&PRIVATE(history)->next_in)) {
		return false;
	}

	// don't let UI thread access data while it's being changed
	mutex_lock(&PRIVATE(history)->mtx_ui_access);

	HistoryIncoming *in = &history->incoming[PRIVATE(history)->first_out];

	for (unsigned int blk = 0; blk < SIGNAL_BLOCKS; ++blk) {

		for (uint64_t changed = in->signals_changed[blk]; changed; changed &= changed - 1) {
			int32_t idx = bit_lowest_set(changed);
			size_t signal = (blk << 6) + (size_t) idx;

			signal_history_store_data(history, signal, in->time, FLAG_IS_SET(in->signals_value[blk], 1ull << idx));
		}
	}

	atomic_store(&PRIVATE(history)->first_out, next_index(history, PRIVATE(history)->first_out));
	mutex_unlock(&PRIVATE(history)->mtx_ui_access);
	return true;
}
