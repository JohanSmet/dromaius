// signal_types.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Basic signal types (inline functions in another header)

#ifndef DROMAIUS_SIGNAL_TYPES_H
#define DROMAIUS_SIGNAL_TYPES_H

#include "types.h"

typedef struct Signal {
	uint16_t	index;
	uint8_t		block;
	uint8_t		layer;
} Signal;

typedef enum SignalValue {
	SV_LOW = 0,
	SV_HIGH = 1,
	SV_HIGH_Z = 2,
} SignalValue;

typedef Signal **SignalGroup;					// dynamic array of Signal *

typedef struct SignalBreakpoint {
	Signal		signal;							// signal to monitor
	bool		pos_edge;						// break on positive edge
	bool		neg_edge;						// break on negative edge
} SignalBreakpoint;

typedef struct {
	Signal		signal;							// signal to write to
	uint64_t	writer_mask;					// id-mask of the chip writing to the signal
	bool		new_value;						// value to write
} SignalQueueEntry;

typedef struct {
	size_t				size;
	SignalQueueEntry *	queue;
} SignalQueue;

 #endif // DROMAIUS_SIGNAL_TYPES_H
