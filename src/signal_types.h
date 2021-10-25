// signal_types.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Basic signal types (inline functions in another header)

#ifndef DROMAIUS_SIGNAL_TYPES_H
#define DROMAIUS_SIGNAL_TYPES_H

#include "types.h"

typedef uint32_t Signal;
typedef Signal *SignalGroup;					// dynamic array of Signal

typedef struct SignalBreakpoint {
	Signal		signal;							// signal to monitor
	bool		pos_edge;						// break on positive edge
	bool		neg_edge;						// break on negative edge
} SignalBreakpoint;

 #endif // DROMAIUS_SIGNAL_TYPES_H
