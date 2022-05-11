// signal_history.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal_history.h"
#include "signal_pool.h"
#include "signal_line.h"

#include "crt.h"
#include "sys/atomics.h"
#include "sys/threads.h"

#include <stb/stb_ds.h>

#ifdef DMS_GTKWAVE_EXPORT
#include <gtkwave/lxt_write.h>
#endif // DMS_GTKWAVE_EXPORT

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

typedef struct HistoryIncoming {
	int64_t		time;
	uint64_t	signals_value[SIGNAL_BLOCKS];
	uint64_t	signals_changed[SIGNAL_BLOCKS];
} HistoryIncoming;

typedef struct SignalHistory_private {
	SignalHistory	public;

	// incoming queue control
	atomic_uint32_t	next_in;
	atomic_uint32_t	first_out;

	// thread control
	bool			thread_stop_request;
	thread_t		thread;

	mutex_t			mtx_work;
	cond_t			cnd_work;

	flag_t			lock_ui_access;

	bool			force_capture_all;

	// gtkwave export
	bool			gtkwave_enabled;
	int64_t			timestep_duration_ps;

	struct lt_trace	*	lxt;
	struct lt_symbol ** lxt_symbols;

	// profiles
	char **				profile_names;				// dynamic array
	Signal **			profile_signals;			// dynamic array
	char ***			profile_signal_aliases;		// dynamic array

} SignalHistory_private;

#define PRIVATE(cpu)	((SignalHistory_private *) (cpu))

///////////////////////////////////////////////////////////////////////////////
//
// private functions
//

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

	int32_t no_work_count = 0;

	while (!PRIVATE(history)->thread_stop_request) {
		if (signal_history_process_incoming_single(history)) {
			no_work_count = 0;
		} else {
			no_work_count += 1;
		}

		if (no_work_count > 1000) {
			// to many sequential iterations without doing work, sleep
			mutex_lock(&PRIVATE(history)->mtx_work);

			// wait for work
			while (!PRIVATE(history)->thread_stop_request && PRIVATE(history)->first_out == atomic_load_uint32(&PRIVATE(history)->next_in)) {
				cond_wait(&PRIVATE(history)->cnd_work, &PRIVATE(history)->mtx_work);
			}

			mutex_unlock(&PRIVATE(history)->mtx_work);
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// gtkwave export utility functions
//

#ifdef DMS_GTKWAVE_EXPORT

static void gtkwave_lxt_open(SignalHistory *history, const char *filename) {

	PRIVATE(history)->lxt = lt_init(filename);
	assert(PRIVATE(history)->lxt);						// FIXME: decent error-handling

	// map signals -> symbols
	PRIVATE(history)->lxt_symbols = dms_calloc(history->signal_count, sizeof(struct lt_symbol *));

	// set dump timescale to picoseconds
	lt_set_timescale(PRIVATE(history)->lxt, -12);
}

static void gtkwave_lxt_start(SignalHistory *history) {
	flag_acquire_lock(&PRIVATE(history)->lock_ui_access);
	PRIVATE(history)->gtkwave_enabled = true;
	flag_release_lock(&PRIVATE(history)->lock_ui_access);
}

static void gtkwave_lxt_close(SignalHistory *history) {

	flag_acquire_lock(&PRIVATE(history)->lock_ui_access);
	PRIVATE(history)->gtkwave_enabled = false;
	flag_release_lock(&PRIVATE(history)->lock_ui_access);

	lt_close(PRIVATE(history)->lxt);
	dms_free(PRIVATE(history)->lxt_symbols);
}

static void gtkwave_enable_signal(SignalHistory *history, size_t signal_idx, const char *signal_name) {
	PRIVATE(history)->lxt_symbols[signal_idx] = lt_symbol_add(PRIVATE(history)->lxt, signal_name, 0, 0, 0, LT_SYM_F_BITS);
}


static void gtkwave_mark_timestep(SignalHistory *history, int64_t time) {
	lt_set_time64(PRIVATE(history)->lxt, (lxttime_t) (time * PRIVATE(history)->timestep_duration_ps));
}

static void gtkwave_trace_value(SignalHistory *history, size_t signal_idx, bool value) {
	if (PRIVATE(history)->lxt_symbols[signal_idx]) {
		lt_emit_value_int(PRIVATE(history)->lxt, PRIVATE(history)->lxt_symbols[signal_idx], 0, value);
	}
}

#endif // DMS_GTKWAVE_EXPORT

///////////////////////////////////////////////////////////////////////////////
//
// interface
//

SignalHistory *signal_history_create(size_t incoming_count, size_t signal_count, size_t sample_count, int64_t timestep_duration) {
	SignalHistory_private *priv = (SignalHistory_private *) dms_calloc(1, sizeof(SignalHistory_private));
	SignalHistory *history = &priv->public;

	history->incoming_count = (unsigned int) incoming_count;
	history->incoming = (HistoryIncoming *) dms_malloc(sizeof(HistoryIncoming) * incoming_count);

	history->signal_count = signal_count;
	history->sample_count = sample_count;
	history->signal_samples_base = (size_t *) dms_calloc(3 * signal_count, sizeof(size_t));
	history->signal_samples_head = history->signal_samples_base + signal_count;
	history->signal_samples_tail = history->signal_samples_head + signal_count;

	history->samples_time = (int64_t *) dms_malloc(sizeof(int64_t) * signal_count * sample_count);
	history->samples_value = (bool *) dms_malloc(sizeof(bool) * signal_count * sample_count);

	for (size_t si = 0; si < signal_count; ++si) {
		history->signal_samples_base[si] = si * sample_count;
		history->signal_samples_head[si] = (size_t) -1;
		history->signal_samples_tail[si] = (size_t) -1;
	}

	priv->timestep_duration_ps = timestep_duration;

	// private variables
	mutex_init_plain(&priv->mtx_work);
	cond_init(&priv->cnd_work);

	return history;
}

void signal_history_destroy(SignalHistory *history) {
	assert(history);
	dms_free(history->incoming);
	dms_free(history->signal_samples_base);
	dms_free(history->samples_time);
	dms_free(history->samples_value);

	for (size_t i = 0; i < arrlenu(PRIVATE(history)->profile_names); ++i) {
		dms_free((char *) PRIVATE(history)->profile_names[i]);
	}
	for (size_t i = 0; i < arrlenu(PRIVATE(history)->profile_signals); ++i) {
		arrfree(PRIVATE(history)->profile_signals[i]);

		for (size_t j = 0; j < arrlenu(PRIVATE(history)->profile_signal_aliases[i]); ++j) {
			dms_free(PRIVATE(history)->profile_signal_aliases[i][j]);
		}
		arrfree(PRIVATE(history)->profile_signal_aliases[i]);
	}
	arrfree(PRIVATE(history)->profile_names);
	arrfree(PRIVATE(history)->profile_signals);
	arrfree(PRIVATE(history)->profile_signal_aliases);
	dms_free(history);
}

void signal_history_process_start(SignalHistory *history) {
	assert(history);

	PRIVATE(history)->thread_stop_request = false;
	PRIVATE(history)->force_capture_all = true;
	history->capture_active = true;
	thread_create_joinable(&PRIVATE(history)->thread, (thread_func_t) signal_history_processing_thread, PRIVATE(history));
}

void signal_history_process_stop(SignalHistory *history) {
	assert(history);

	mutex_lock(&PRIVATE(history)->mtx_work);
	PRIVATE(history)->thread_stop_request = true;
	mutex_unlock(&PRIVATE(history)->mtx_work);

#ifdef DMS_GTKWAVE_EXPORT
	if (PRIVATE(history)->gtkwave_enabled) {
		signal_history_gtkwave_disable(history);
	}
#endif // DMS_GTKWAVE_EXPORT

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

bool signal_history_add(SignalHistory *history, int64_t time, uint64_t *signals_value, uint64_t *signals_changed, bool block) {
	// runs on the simulator thread
	assert(history);

	// don't let next_in index 'catch up' with first_out index
	while (next_index(history, PRIVATE(history)->next_in) == atomic_load_uint32(&PRIVATE(history)->first_out)) {
		if (!block) {
			return false;
		}
		// spin-lock: first_out is updated in another thread
		cond_signal(&PRIVATE(history)->cnd_work);
	}

	// copy values
	HistoryIncoming *in = &history->incoming[PRIVATE(history)->next_in];
	in->time = time;
	dms_memcpy(in->signals_value, signals_value, sizeof(uint64_t) * SIGNAL_BLOCKS);
	if (!PRIVATE(history)->force_capture_all) {
		dms_memcpy(in->signals_changed, signals_changed, sizeof(uint64_t) * SIGNAL_BLOCKS);
	} else {
		size_t signals_left = history->signal_count;
		for (size_t i = 0; i < SIGNAL_BLOCKS; ++i, signals_left -= 64) {
			in->signals_changed[i] = (signals_left > 64) ? (uint64_t) -1 : (1ull << signals_left) - 1;
		}
		PRIVATE(history)->force_capture_all = false;
	}

	// move pointer along -- there's only one thread writing to next_in
	atomic_exchange_uint32(&PRIVATE(history)->next_in, next_index(history, PRIVATE(history)->next_in));

	// kick worker thread awake if necessary
	cond_signal(&PRIVATE(history)->cnd_work);

	return true;
}

void signal_history_diagram_data(struct SignalHistory *history, SignalHistoryDiagramData *diagram_data) {
	assert(history);
	assert(diagram_data);

	// clear any data that's already in there
	prepare_diagram_data(diagram_data);

	flag_acquire_lock(&PRIVATE(history)->lock_ui_access);

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

	flag_release_lock(&PRIVATE(history)->lock_ui_access);
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

	// gtkwave export
#ifdef DMS_GTKWAVE_EXPORT
	if (PRIVATE(history)->gtkwave_enabled) {
		gtkwave_trace_value(history, signal, value);
	}
#endif // DMS_GTKWAVE_EXPORT
}

bool signal_history_process_incoming_single(SignalHistory *history) {
	// runs on the dedicated history thread
	assert(history);

	// check if there is work to do
	if (PRIVATE(history)->first_out == atomic_load_uint32(&PRIVATE(history)->next_in)) {
		return false;
	}

	// don't let UI thread access data while it's being changed
	flag_acquire_lock(&PRIVATE(history)->lock_ui_access);

	HistoryIncoming *in = &history->incoming[PRIVATE(history)->first_out];

#ifdef DMS_GTKWAVE_EXPORT
	if (PRIVATE(history)->gtkwave_enabled) {
		gtkwave_mark_timestep(history, in->time);
	}
#endif // DMS_GTKWAVE_EXPORT

	for (unsigned int blk = 0; blk < SIGNAL_BLOCKS; ++blk) {

		for (uint64_t changed = in->signals_changed[blk]; changed; changed &= changed - 1) {
			int32_t idx = bit_lowest_set(changed);
			size_t signal = (blk << 6) + (size_t) idx;
			signal_history_store_data(history, signal, in->time, FLAG_IS_SET(in->signals_value[blk], 1ull << idx));
		}
	}

	flag_release_lock(&PRIVATE(history)->lock_ui_access);
	atomic_exchange_uint32(&PRIVATE(history)->first_out, next_index(history, PRIVATE(history)->first_out));
	return true;
}

#ifdef DMS_GTKWAVE_EXPORT

void signal_history_gtkwave_enable(SignalHistory *history, const char *filename, size_t count, Signal *signals, char **signal_names) {
	assert(history);
	gtkwave_lxt_open(history, filename);

	for (size_t i = 0; i < count; ++i) {
		size_t signal_idx = signal_array_subscript(signals[i]);
		gtkwave_enable_signal(history, signal_idx, signal_names[signal_idx]);
	}

	gtkwave_lxt_start(history);
}

void signal_history_gtkwave_disable(SignalHistory *history) {
	assert(history);
	gtkwave_lxt_close(history);
}

#endif // DMS_GTKWAVE_EXPORT

uint32_t signal_history_profile_create(SignalHistory *history, const char *chip_name, const char *profile_name) {
	assert(history);
	assert(arrlen(PRIVATE(history)->profile_names) == arrlen(PRIVATE(history)->profile_signals));

	// concatenate name
	const char *separator = " - ";
	size_t len = dms_strlen(chip_name) + dms_strlen(profile_name) + dms_strlen(separator) + 1;
	char *full_name = dms_malloc(len);
	assert(full_name);
	dms_strlcpy(full_name, chip_name, len);
	dms_strlcat(full_name, separator, len);
	dms_strlcat(full_name, profile_name, len);

	arrpush(PRIVATE(history)->profile_names, full_name);
	arrpush(PRIVATE(history)->profile_signals, NULL);
	arrpush(PRIVATE(history)->profile_signal_aliases, NULL);

	return (uint32_t) (arrlen(PRIVATE(history)->profile_names) - 1);
}

void signal_history_profile_add_signal(SignalHistory *history, uint32_t profile, Signal signal, const char *alias) {
	assert(history);
	assert(profile < arrlen(PRIVATE(history)->profile_signals));

	arrpush(PRIVATE(history)->profile_signals[profile], signal);
	arrpush(PRIVATE(history)->profile_signal_aliases[profile], (alias) ? dms_strdup(alias) : NULL);
}

const char **signal_history_profile_names(SignalHistory *history) {
	assert(history);
	return (const char **) PRIVATE(history)->profile_names;
}

size_t signal_history_profile_count(SignalHistory *history) {
	assert(history);
	return arrlenu(PRIVATE(history)->profile_names);
}

Signal *signal_history_profile_signals(SignalHistory *history, uint32_t profile) {
	assert(history);
	assert(profile < arrlen(PRIVATE(history)->profile_signals));
	return PRIVATE(history)->profile_signals[profile];
}

const char **signal_history_profile_signal_aliases(SignalHistory *history, uint32_t profile) {
	assert(history);
	assert(profile < arrlen(PRIVATE(history)->profile_signals));
	return (const char **) PRIVATE(history)->profile_signal_aliases[profile];
}
