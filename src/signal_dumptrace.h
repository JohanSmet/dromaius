// signal_dumptrace.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_SIGNAL_DUMPTRACE_H
#define DROMAIUS_SIGNAL_DUMPTRACE_H

#include "types.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// forward declarations
struct SignalTrace;

// interface
struct SignalTrace *signal_trace_open(const char *filename, SignalPool *signal_pool);
void signal_trace_close(struct SignalTrace *trace);

void signal_trace_enable_signal(struct SignalTrace *trace, Signal signal);

void signal_trace_mark_timestep(struct SignalTrace *trace);
void signal_trace_value(struct SignalTrace *trace, Signal signal);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIGNAL_DUMPTRACE_H
