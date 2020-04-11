// signal_line.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal_line.h"

#include <stb/stb_ds.h>

#include <assert.h>
#include <stdlib.h>

static inline uint8_t read_uint8(bool *src, size_t len) {
	uint8_t result = 0;
	for (size_t i = 0; i < len; ++i) {
		result |= (uint8_t) (src[i] << i);
	}

	return result;
}

static inline uint16_t read_uint16(bool *src, size_t len) {
	uint16_t result = 0;
	for (size_t i = 0; i < len; ++i) {
		result |= (uint16_t) (src[i] << i);
	}

	return result;
}

SignalPool *signal_pool_create(void) {
	SignalPool *pool = (SignalPool *) calloc(1, sizeof(SignalPool));
	return pool;
}

void signal_pool_destroy(SignalPool *pool) {
	assert(pool);

	arrfree(pool->signals_curr);
	arrfree(pool->signals_next);
	arrfree(pool->signals_default);
	free(pool);
}

void signal_pool_cycle(SignalPool *pool) {
	assert(pool);
	assert(arrlenu(pool->signals_curr) == arrlenu(pool->signals_next));
	assert(arrlenu(pool->signals_default) == arrlenu(pool->signals_next));

	memcpy(pool->signals_curr, pool->signals_next, arrlenu(pool->signals_next));
	memcpy(pool->signals_next, pool->signals_default, arrlenu(pool->signals_default));
}

Signal signal_create(SignalPool *pool, uint32_t size) {
	assert(pool);
	assert(size > 0);

	Signal result = {(uint32_t) arrlenu(pool->signals_curr), size};

	for (uint32_t i = 0; i < size; ++i) {
		arrpush(pool->signals_curr, false);
		arrpush(pool->signals_next, false);
		arrpush(pool->signals_default, false);
	}

	return result;
}

Signal signal_split(Signal src, uint32_t start, uint32_t size) {
	assert(start < src.count);
	assert(start + size <= src.count);

	return (Signal){src.start + start, size};
}

void signal_default_bool(SignalPool *pool, Signal signal, bool value) {
	assert(pool);
	assert(signal.count == 1);

	pool->signals_default[signal.start] = value;
	pool->signals_curr[signal.start] = value;
	pool->signals_next[signal.start] = value;
}

void signal_default_uint8(SignalPool *pool, Signal signal, uint8_t value) {
	assert(pool);
	assert(signal.count <= 8);

	for (uint32_t i = 0; i < signal.count; ++i) {
		pool->signals_default[signal.start + i] = value & 1;
		pool->signals_curr[signal.start + i] = value & 1;
		pool->signals_next[signal.start + i] = value & 1;
		value >>= 1;
	}
}

void signal_default_uint16(SignalPool *pool, Signal signal, uint16_t value) {
	assert(pool);
	assert(signal.count <= 16);

	for (uint32_t i = 0; i < signal.count; ++i) {
		pool->signals_default[signal.start + i] = value & 1;
		pool->signals_curr[signal.start + i] = value & 1;
		pool->signals_next[signal.start + i] = value & 1;
		value >>= 1;
	}
}

bool signal_read_bool(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count == 1);

	return pool->signals_curr[signal.start];
}

uint8_t signal_read_uint8(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 8);

	return read_uint8(pool->signals_curr + signal.start, signal.count);
}

uint16_t signal_read_uint16(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 16);

	return read_uint16(pool->signals_curr + signal.start, signal.count);
}

void signal_write_bool(SignalPool *pool, Signal signal, bool value) {
	assert(pool);
	assert(signal.count == 1);
	pool->signals_next[signal.start] = value;
}

void signal_write_uint8(SignalPool *pool, Signal signal, uint8_t value) {
	assert(pool);
	assert(signal.count <= 8);

	for (uint32_t i = 0; i < signal.count; ++i) {
		pool->signals_next[signal.start + i] = value & 1;
		value >>= 1;
	}
}

void signal_write_uint16(SignalPool *pool, Signal signal, uint16_t value) {
	assert(pool);
	assert(signal.count <= 16);

	for (uint32_t i = 0; i < signal.count; ++i) {
		pool->signals_next[signal.start + i] = value & 1;
		value >>= 1;
	}
}

void signal_write_uint8_masked(SignalPool *pool, Signal signal, uint8_t value, uint8_t mask) {
	assert(pool);
	assert(signal.count <= 8);

	for (uint32_t i = 0; i < signal.count; ++i) {
		uint8_t b = mask & 1;

		pool->signals_next[signal.start + i] = (pool->signals_next[signal.start + i] & ~b) | (value & b);
		mask >>= 1;
		value >>= 1;
	}
}

void signal_write_uint16_masked(SignalPool *pool, Signal signal, uint16_t value, uint16_t mask) {
	assert(pool);
	assert(signal.count <= 16);

	for (uint32_t i = 0; i < signal.count; ++i) {
		uint8_t b = mask & 1;

		pool->signals_next[signal.start + i] = (pool->signals_next[signal.start + i] & ~b) | (value & b);
		mask >>= 1;
		value >>= 1;
	}
}

bool signal_read_next_bool(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count == 1);

	return pool->signals_next[signal.start];
}

uint8_t signal_read_next_uint8(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 8);

	return read_uint8(pool->signals_next + signal.start, signal.count);
}

uint16_t signal_read_next_uint16(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 16);

	return read_uint16(pool->signals_next + signal.start, signal.count);
}
