// signal_line.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// NOTE: some functions provide a fast path when the signal aligns with a 64-bit integer (an 8-bit signal).
//		 Further improvements can be had by force-aligning these signals at creating and defining read/write functions
//		 that forgo these runtime checks.

#ifndef DROMAIUS_SIGNAL_LINE_H
#define DROMAIUS_SIGNAL_LINE_H

#include "types.h"
#include <assert.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct Signal {
	uint32_t	start;
	uint32_t	count;
	int8_t		domain;
} Signal;

typedef struct SignalDomain {
	bool *		signals_curr;
	bool *		signals_next;
	bool *		signals_default;
	char *		signals_name;
} SignalDomain;

typedef struct SignalNameMap {
	const char *key;
	Signal		value;
} SignalNameMap;

typedef struct SignalPool {
	int8_t			default_domain;
	int8_t			num_domains;
	SignalNameMap	*signal_names;
	SignalDomain	domains[0];
} SignalPool;

extern const uint64_t lut_bit_to_byte[256];

#define MAX_SIGNAL_NAME		8

// functions
SignalPool *signal_pool_create(size_t num_domains);
void signal_pool_destroy(SignalPool *pool);
void signal_pool_current_domain(SignalPool *pool, int8_t domain);

void signal_pool_cycle(SignalPool *pool);
void signal_pool_cycle_domain(SignalPool *pool, int8_t domain);
void signal_pool_cycle_domain_no_reset(SignalPool *pool, int8_t domain);

Signal signal_create(SignalPool *pool, uint32_t size);
void signal_set_name(SignalPool *pool, Signal Signal, const char *name);
const char * signal_get_name(SignalPool *pool, Signal signal);

void signal_default_bool(SignalPool *pool, Signal signal, bool value);
void signal_default_uint8(SignalPool *pool, Signal signal, uint8_t value);
void signal_default_uint16(SignalPool *pool, Signal signal, uint16_t value);

static inline Signal signal_split(Signal src, uint32_t start, uint32_t size) {
	assert(start < src.count);
	assert(start + size <= src.count);

	Signal result = {src.start + start, size, src.domain};
	return result;
}

Signal signal_by_name(SignalPool *pool, const char *name);

static inline bool signal_read_bool(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count == 1);

	return pool->domains[signal.domain].signals_curr[signal.start];
}

static inline uint8_t signal_read_uint8_internal(bool *src, size_t len) {
	uint8_t result = 0;
	for (size_t i = 0; i < len; ++i) {
		result |= (uint8_t) (src[i] << i);
	}
	return result;
}

static inline uint16_t signal_read_uint16_internal(bool *src, size_t len) {
	uint16_t result = 0;
	for (size_t i = 0; i < len; ++i) {
		result |= (uint16_t) (src[i] << i);
	}
	return result;
}

static inline uint8_t signal_read_uint8(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 8);

	return signal_read_uint8_internal(pool->domains[signal.domain].signals_curr + signal.start, signal.count);
}

static inline uint16_t signal_read_uint16(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 16);

	return signal_read_uint16_internal(pool->domains[signal.domain].signals_curr + signal.start, signal.count);
}

static inline void signal_write_bool(SignalPool *pool, Signal signal, bool value) {
	assert(pool);
	assert(signal.count == 1);
	pool->domains[signal.domain].signals_next[signal.start] = value;
}

static inline void signal_write_uint8(SignalPool *pool, Signal signal, uint8_t value) {
	assert(pool);
	assert(signal.count <= 8);

	if (signal.count == 8 && (signal.start & 0x7) == 0) {
		(*(uint64_t *) &pool->domains[signal.domain].signals_next[signal.start]) = lut_bit_to_byte[value];
	} else {
		memcpy(&pool->domains[signal.domain].signals_next[signal.start], &lut_bit_to_byte[value], signal.count);
	}
}

static inline void signal_write_uint16(SignalPool *pool, Signal signal, uint16_t value) {
	assert(pool);
	assert(signal.count <= 16);

	for (uint32_t i = 0; i < signal.count; ++i) {
		pool->domains[signal.domain].signals_next[signal.start + i] = value & 1;
		value = (uint16_t) (value >> 1);
	}
}

static inline void signal_write_uint8_masked(SignalPool *pool, Signal signal, uint8_t value, uint8_t mask) {
	assert(pool);
	assert(signal.count <= 8);

	bool *signals_next = pool->domains[signal.domain].signals_next;

	if (signal.count == 8 && (signal.start & 0x7) == 0) {
		uint64_t byte_mask = lut_bit_to_byte[mask];
		uint64_t current   = (*(uint64_t *) &signals_next[signal.start]);
		(*(uint64_t *) &signals_next[signal.start]) = (current & ~byte_mask) | (lut_bit_to_byte[value] & byte_mask);
	} else {
		for (uint32_t i = 0; i < signal.count; ++i) {
			bool b = (mask >> i) & 1;
			if (b) {
				signals_next[signal.start + i] = (value >> i) & 1;
			}
		}
	}
}

static inline void signal_write_uint16_masked(SignalPool *pool, Signal signal, uint16_t value, uint16_t mask) {
	assert(pool);
	assert(signal.count <= 16);

	for (uint32_t i = 0; i < signal.count; ++i) {
		uint8_t b = (mask >> i) & 1;
		if (b) {
			pool->domains[signal.domain].signals_next[signal.start + i] = (value >> i) & 1;
		}
	}
}

static inline bool signal_read_next_bool(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count == 1);

	return pool->domains[signal.domain].signals_next[signal.start];
}

static inline uint8_t signal_read_next_uint8(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 8);

	return signal_read_uint8_internal(pool->domains[signal.domain].signals_next + signal.start, signal.count);
}

static inline uint16_t signal_read_next_uint16(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 16);

	return signal_read_uint16_internal(pool->domains[signal.domain].signals_next + signal.start, signal.count);
}

static inline bool signal_changed(SignalPool *pool, Signal signal) {
	assert(pool);

	SignalDomain *domain = &pool->domains[signal.domain];
	return memcmp(domain->signals_curr + signal.start, domain->signals_next + signal.start, signal.count) != 0;
}

// macros to make working with signal a little prettier
//	define SIGNAL_POOL and SIGNAL_COLLECTION in your source file

#define SIGNAL_DEFINE(sig,cnt)										\
	if (SIGNAL_COLLECTION.sig.count == 0) {							\
		SIGNAL_COLLECTION.sig = signal_create(SIGNAL_POOL, (cnt));	\
	}

#define SIGNAL_DEFINE_N(sig,cnt,name)									\
	if (SIGNAL_COLLECTION.sig.count == 0) {								\
		SIGNAL_COLLECTION.sig = signal_create(SIGNAL_POOL, (cnt));		\
		signal_set_name(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (name));	\
	}

#define SIGNAL_DEFINE_BOOL(sig,cnt,def)										\
	if (SIGNAL_COLLECTION.sig.count == 0) {									\
		SIGNAL_COLLECTION.sig = signal_create(SIGNAL_POOL, (cnt));			\
		signal_default_bool(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (def));		\
	}

#define SIGNAL_DEFINE_BOOL_N(sig,cnt,def,name)								\
	if (SIGNAL_COLLECTION.sig.count == 0) {									\
		SIGNAL_COLLECTION.sig = signal_create(SIGNAL_POOL, (cnt));			\
		signal_default_bool(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (def));		\
		signal_set_name(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (name));		\
	}

#define SIGNAL(sig)		SIGNAL_COLLECTION.sig

#define SIGNAL_BOOL(sig)	signal_read_bool(SIGNAL_POOL, SIGNAL_COLLECTION.sig)
#define SIGNAL_UINT8(sig)	signal_read_uint8(SIGNAL_POOL, SIGNAL_COLLECTION.sig)
#define SIGNAL_UINT16(sig)	signal_read_uint16(SIGNAL_POOL, SIGNAL_COLLECTION.sig)

#define SIGNAL_NEXT_BOOL(sig)	signal_read_next_bool(SIGNAL_POOL, SIGNAL_COLLECTION.sig)
#define SIGNAL_NEXT_UINT8(sig)	signal_read_next_uint8(SIGNAL_POOL, SIGNAL_COLLECTION.sig)
#define SIGNAL_NEXT_UINT16(sig)	signal_read_next_uint16(SIGNAL_POOL, SIGNAL_COLLECTION.sig)

#define SIGNAL_SET_BOOL(sig,v)		signal_write_bool(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (v))
#define SIGNAL_SET_UINT8(sig,v)		signal_write_uint8(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (v))
#define SIGNAL_SET_UINT16(sig,v)	signal_write_uint16(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (v))

#define SIGNAL_SET_UINT8_MASKED(sig,v,m)	signal_write_uint8_masked(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (v), (m))
#define SIGNAL_SET_UINT16_MASKED(sig,v,m)	signal_write_uint16_masked(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (v), (m))

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIGNAL_LINE_H
