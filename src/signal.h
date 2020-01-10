// signal.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_SIGNAL_H
#define DROMAIUS_SIGNAL_H

#include "types.h"

// types
typedef struct Signal {
	uint32_t	start;
	uint32_t	count;
} Signal;

typedef struct SignalPool {
	bool *		signals_curr;
	bool *		signals_next;
	bool *		signals_default;
} SignalPool;

// functions
SignalPool *signal_pool_create(void);
void signal_pool_destroy(SignalPool *pool);
void signal_pool_cycle(SignalPool *pool);

Signal signal_create(SignalPool *pool, size_t size);
Signal signal_split(Signal src, size_t start, size_t size);

void signal_default_bool(SignalPool *pool, Signal signal, bool value);
void signal_default_uint8(SignalPool *pool, Signal signal, uint8_t value);
void signal_default_uint16(SignalPool *pool, Signal signal, uint16_t value);

bool signal_read_bool(SignalPool *pool, Signal signal);
uint8_t signal_read_uint8(SignalPool *pool, Signal signal);
uint16_t signal_read_uint16(SignalPool *pool, Signal signal);

void signal_write_bool(SignalPool *pool, Signal signal, bool value);
void signal_write_uint8(SignalPool *pool, Signal signal, uint8_t value);
void signal_write_uint16(SignalPool *pool, Signal signal, uint16_t value);

void signal_write_uint8_masked(SignalPool *pool, Signal signal, uint8_t value, uint8_t mask);
void signal_write_uint16_masked(SignalPool *pool, Signal signal, uint16_t value, uint16_t mask);

bool signal_read_next_bool(SignalPool *pool, Signal signal);
uint8_t signal_read_next_uint8(SignalPool *pool, Signal signal);
uint16_t signal_read_next_uint16(SignalPool *pool, Signal signal);

#endif //  DROMAIUS_SIGNAL_H
