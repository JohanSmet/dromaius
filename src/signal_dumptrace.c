// signal_dumptrace.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal_dumptrace.h"

#include <gtkwave/lxt_write.h>
#include <stb/stb_ds.h>

#include <stdlib.h>
#include <assert.h>

// types
typedef struct SignalTrace {
	SignalPool *signal_pool;

	struct lt_trace	*	lxt;
	struct lt_symbol ** symbols;

	int64_t		timestep_duration_ps;
} SignalTrace;

// interface
SignalTrace *signal_trace_open(const char *filename, SignalPool *signal_pool) {
	SignalTrace *trace = (SignalTrace *) calloc(1, sizeof(SignalTrace));

	trace->signal_pool = signal_pool;
	trace->lxt = lt_init(filename);
	assert(trace->lxt);						// FIXME: decent error-handling

	// map signals -> symbols
	arrsetlen(trace->symbols, arrlenu(signal_pool->signals_name));
	memset(trace->symbols, 0, arrlenu(trace->symbols) * sizeof(struct lt_symbol *));

	// set dump timescale to picoseconds
	lt_set_timescale(trace->lxt, -12);

	return trace;
}

void signal_trace_close(struct SignalTrace *trace) {
	assert(trace);

	lt_close(trace->lxt);

	arrfree(trace->symbols);
	free(trace);
}

void signal_trace_set_timestep_duration(SignalTrace *trace, int64_t timestep_duration_ps) {
	assert(trace);
	trace->timestep_duration_ps = timestep_duration_ps;
}

void signal_trace_enable_signal(struct SignalTrace *trace, Signal signal) {
	assert(trace);
	assert(trace->symbols[signal.start] == NULL);

	trace->symbols[signal.start] = lt_symbol_add(trace->lxt, trace->signal_pool->signals_name[signal.start], 0, 0, 0, LT_SYM_F_BITS);
	lt_emit_value_int(trace->lxt, trace->symbols[signal.start], 0, trace->signal_pool->signals_curr[signal.start]);
	assert(trace->symbols[signal.start]);
}

void signal_trace_mark_timestep(struct SignalTrace *trace, int64_t current_time_ps) {
	if (trace) {
		lt_set_time64(trace->lxt, (lxttime_t) (current_time_ps * trace->timestep_duration_ps));
	}
}

void signal_trace_value(struct SignalTrace *trace, Signal signal) {
	if (trace && trace->symbols[signal.start]) {
		lt_emit_value_int(trace->lxt, trace->symbols[signal.start], 0, trace->signal_pool->signals_curr[signal.start]);
	}
}
