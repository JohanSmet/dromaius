// signal_history.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_SIGNAL_HISTORY_H
#define DROMAIUS_SIGNAL_HISTORY_H

#include "signal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct SignalHistory {
	bool					capture_active;

	unsigned int			incoming_count;
	struct HistoryIncoming 	*incoming;

	size_t					signal_count;
	size_t					sample_count;
	size_t *				signal_samples_base;		// fixed start of signal samples in the buffers
	size_t *				signal_samples_head;		// offset of newest sample for signal (relative to base)
	size_t *				signal_samples_tail;		// offset of oldest sample for signal (relative to base)
	int64_t *				samples_time;
	bool *					samples_value;
} SignalHistory;

typedef struct SignalHistoryDiagramData {
	Signal *				signals;					// dynamic array
	int64_t					time_begin;
	int64_t					time_end;

	size_t *				signal_start_offsets;		// dynamic array
	int64_t	*				samples_time;				// dynamic array
	bool *					samples_value;				// dynamic array
} SignalHistoryDiagramData;

// forward declaration of private types
struct SignalHistory;

// interface
struct SignalHistory *signal_history_create(size_t incoming_count, size_t signal_count, size_t sample_count, int64_t timestep_duration);
void signal_history_destroy(struct SignalHistory *history);

// control history processing
void signal_history_process_start(SignalHistory *history);
void signal_history_process_stop(SignalHistory *history);

// signal_history_add: add batch changed signals to the history (called from simulator)
bool signal_history_add(struct SignalHistory *history, int64_t time, uint64_t *signals_value, uint64_t *signals_changed, bool block);

// signal_history_diagram_data: retrieve data to build an logic analyzer display in the UI
void signal_history_diagram_data(SignalHistory *history, SignalHistoryDiagramData *diagram_data);
void signal_history_diagram_release(SignalHistoryDiagramData *diagram_data);

// gtkwave export
#ifdef DMS_GTKWAVE_EXPORT
void signal_history_gtkwave_enable(SignalHistory *history, const char *filename, size_t count, Signal *signals, char **signal_names);
void signal_history_gtkwave_disable(SignalHistory *history);
#endif // DMS_GTKWAVE_EXPORT

// profiles
uint32_t signal_history_profile_create(SignalHistory *history, const char *chip_name, const char *profile_name);
void signal_history_profile_add_signal(SignalHistory *history, uint32_t profile, Signal signal, const char *alias);

const char **signal_history_profile_names(SignalHistory *history);
size_t signal_history_profile_count(SignalHistory *history);
Signal *signal_history_profile_signals(SignalHistory *history, uint32_t profile);
const char **signal_history_profile_signal_aliases(SignalHistory *history, uint32_t profile);

// private interface -- exposed for unit tests
bool signal_history_process_incoming_single(SignalHistory *history);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIGNAL_HISTORY_H

