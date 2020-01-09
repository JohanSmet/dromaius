// signal.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal.h"

#include <stb/stb_ds.h>

#include <assert.h>
#include <stdlib.h>

SignalPool *signal_pool_create(void) {
	SignalPool *pool = (SignalPool *) calloc(1, sizeof(SignalPool));
	return pool;
}

void signal_pool_destroy(SignalPool *pool) {
	assert(pool);

	arrfree(pool->signals_read);
	arrfree(pool->signals_write);
	free(pool);
}

void signal_pool_cycle(SignalPool *pool) {
	assert(pool);
	assert(arrlen(pool->signals_read) == arrlen(pool->signals_write));

	memcpy(pool->signals_read, pool->signals_write, arrlen(pool->signals_write));
	memset(pool->signals_write, 0, arrlen(pool->signals_write));
}

Signal signal_create(SignalPool *pool, size_t size) {
	assert(pool);
	assert(size > 0);

	Signal result = {arrlen(pool->signals_read), size};
	
	for (int i = 0; i < size; ++i) {
		arrpush(pool->signals_read, false);
		arrpush(pool->signals_write, false);
	}

	return result;
}

Signal signal_split(Signal src, size_t start, size_t size) {
	assert(start < src.count);
	assert(start + size <= src.count);

	return (Signal){src.start + start, size};
}

bool signal_read_bool(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count == 1);

	return pool->signals_read[signal.start];
}

uint8_t signal_read_uint8(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 8);

	uint8_t result = 0;
	for (int i = 0; i < signal.count; ++i) {
		result |= pool->signals_read[signal.start+i] << i;
	}

	return result;
}

uint16_t signal_read_uint16(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 16);

	uint16_t result = 0;
	for (int i = 0; i < signal.count; ++i) {
		result |= pool->signals_read[signal.start+i] << i;
	}

	return result;
}

void signal_write_bool(SignalPool *pool, Signal signal, bool value) {
	assert(pool);
	assert(signal.count == 1);
	pool->signals_write[signal.start] = value;
}

void signal_write_uint8(SignalPool *pool, Signal signal, uint8_t value) {
	assert(pool);
	assert(signal.count <= 8);

	for (int i = 0; i < signal.count; ++i) {
		pool->signals_write[signal.start + i] = value & 1;
		value >>= 1;
	}
}

void signal_write_uint16(SignalPool *pool, Signal signal, uint16_t value) {
	assert(pool);
	assert(signal.count <= 16);

	for (int i = 0; i < signal.count; ++i) {
		pool->signals_write[signal.start + i] = value & 1;
		value >>= 1;
	}
}

void signal_write_uint8_masked(SignalPool *pool, Signal signal, uint8_t value, uint8_t mask) {
	assert(pool);
	assert(signal.count <= 8);

	for (int i = 0; i < signal.count; ++i) {
		bool b = mask & 1;

		pool->signals_write[signal.start + i] = (pool->signals_read[signal.start + i] & ~b) | (value & b);
		mask >>= 1;
		value >>= 1;
	}
}

void signal_write_uint16_masked(SignalPool *pool, Signal signal, uint16_t value, uint16_t mask) {
	assert(pool);
	assert(signal.count <= 16);

	for (int i = 0; i < signal.count; ++i) {
		bool b = mask & 1;

		pool->signals_write[signal.start + i] = (pool->signals_read[signal.start + i] & ~b) | (value & b);
		mask >>= 1;
		value >>= 1;
	}
}
