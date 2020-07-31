// stopwatch.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// wall-time interface

#ifndef DROMAIUS_STOPWATCH_H
#define DROMAIUS_STOPWATCH_H

#include "types.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct Stopwatch {
	bool		running;
	int64_t		start_timestamp;
} Stopwatch;

// functions
Stopwatch *stopwatch_create();
void stopwatch_destroy(Stopwatch *stopwatch);

void stopwatch_start(Stopwatch *stopwatch);
void stopwatch_stop(Stopwatch *stopwatch);
int64_t stopwatch_time_elapsed_ps(Stopwatch *stopwatch);

void stopwatch_sleep(int64_t interval_ps);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_STOPWATCH_H
