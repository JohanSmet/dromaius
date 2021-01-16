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
#include <stb/stb_ds.h>

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct Signal {
	uint32_t	start;
	uint32_t	count;
} Signal;

typedef struct SignalBreak {
	Signal		signal;							// signal to monitor
	bool		pos_edge;						// break on positive edge
	bool		neg_edge;						// break on negative edge
} SignalBreak;

typedef struct SignalNameMap {
	const char *key;
	Signal		value;
} SignalNameMap;

typedef struct SignalPool {
	bool *			signals_curr;
	bool *			signals_next;
	bool  *			signals_changed;
	uint32_t *		signals_written;
	bool *			signals_default;
	const char **	signals_name;

	uint64_t *		signals_writers;
	uint64_t *		dependent_components;
	uint64_t		rerun_chips;

	SignalNameMap	*signal_names;

	int64_t			tick_last_cycle;

#ifdef DMS_SIGNAL_TRACING
	struct SignalTrace *trace;
#endif
} SignalPool;

extern const uint64_t lut_bit_to_byte[256];

#define MAX_SIGNAL_NAME		8

// functions
SignalPool *signal_pool_create(void);
void signal_pool_destroy(SignalPool *pool);

void signal_pool_cycle(SignalPool *pool, int64_t current_tick);
uint64_t signal_pool_cycle_dirty_flags(SignalPool *pool, int64_t current_tick);

Signal signal_create(SignalPool *pool, uint32_t size);
void signal_set_name(SignalPool *pool, Signal Signal, const char *name);
const char * signal_get_name(SignalPool *pool, Signal signal);
void signal_add_dependency(SignalPool *pool, Signal signal, int32_t chip_id);

void signal_default_bool(SignalPool *pool, Signal signal, bool value);
void signal_default_uint8(SignalPool *pool, Signal signal, uint8_t value);
void signal_default_uint16(SignalPool *pool, Signal signal, uint16_t value);

static inline Signal signal_split(Signal src, uint32_t start, uint32_t size) {
	assert(start < src.count);
	assert(start + size <= src.count);

	Signal result = {src.start + start, size};
	return result;
}

Signal signal_by_name(SignalPool *pool, const char *name);

static inline bool signal_read_bool(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count == 1);

	return pool->signals_curr[signal.start];
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

	return signal_read_uint8_internal(pool->signals_curr + signal.start, signal.count);
}

static inline uint16_t signal_read_uint16(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 16);

	return signal_read_uint16_internal(pool->signals_curr + signal.start, signal.count);
}

static inline void signal_write_bool(SignalPool *pool, Signal signal, bool value, int32_t chip_id) {
	assert(pool);
	assert(signal.count == 1);
	pool->signals_next[signal.start] = value;
	pool->signals_writers[signal.start] |= 1ull << chip_id;
	arrpush(pool->signals_written, signal.start);
}

static inline void signal_write_uint8(SignalPool *pool, Signal signal, uint8_t value, int32_t chip_id) {
	assert(pool);
	assert(signal.count <= 8);

	if (signal.count == 8 && (signal.start & 0x7) == 0) {
		(*(uint64_t *) &pool->signals_next[signal.start]) = lut_bit_to_byte[value];
	} else {
		memcpy(&pool->signals_next[signal.start], &lut_bit_to_byte[value], signal.count);
	}

	for (uint32_t i = 0; i < signal.count; ++i) {
		pool->signals_writers[signal.start + i] |= 1ull << chip_id;
		arrpush(pool->signals_written, signal.start + i);
	}
}

static inline void signal_write_uint16(SignalPool *pool, Signal signal, uint16_t value, int32_t chip_id) {
	assert(pool);
	assert(signal.count <= 16);

	for (uint32_t i = 0; i < signal.count; ++i) {
		pool->signals_next[signal.start + i] = value & 1;
		pool->signals_writers[signal.start + i] |= 1ull << chip_id;
		arrpush(pool->signals_written, signal.start + i);
		value = (uint16_t) (value >> 1);
	}
}

static inline void signal_write_uint8_masked(SignalPool *pool, Signal signal, uint8_t value, uint8_t mask, int32_t chip_id) {
	assert(pool);
	assert(signal.count <= 8);

	bool *signals_next = pool->signals_next;
	uint64_t *signals_writers = pool->signals_writers;

	if (signal.count == 8 && (signal.start & 0x7) == 0) {
		uint64_t byte_mask = lut_bit_to_byte[mask];
		uint64_t current   = (*(uint64_t *) &signals_next[signal.start]);
		(*(uint64_t *) &signals_next[signal.start]) = (current & ~byte_mask) | (lut_bit_to_byte[value] & byte_mask);

		for (uint32_t i = 0; i < signal.count; ++i) {
			bool b = (mask >> i) & 1;
			if (b) {
				signals_writers[signal.start + i] |= 1ull << chip_id;
				arrpush(pool->signals_written, signal.start + i);
			}
		}
	} else {
		for (uint32_t i = 0; i < signal.count; ++i) {
			bool b = (mask >> i) & 1;
			if (b) {
				signals_next[signal.start + i] = (value >> i) & 1;
				signals_writers[signal.start + i] |= 1ull << chip_id;
				arrpush(pool->signals_written, signal.start + i);
			}
		}
	}
}

static inline void signal_write_uint16_masked(SignalPool *pool, Signal signal, uint16_t value, uint16_t mask, int32_t chip_id) {
	assert(pool);
	assert(signal.count <= 16);

	for (uint32_t i = 0; i < signal.count; ++i) {
		uint8_t b = (mask >> i) & 1;
		if (b) {
			pool->signals_next[signal.start + i] = (value >> i) & 1;
			pool->signals_writers[signal.start + i] |= 1ull << chip_id;
			arrpush(pool->signals_written, signal.start + i);
		}
	}
}

static inline void signal_clear_writer(SignalPool *pool, Signal signal, int32_t chip_id) {
	assert(pool);

	uint64_t dep_mask = 1ull << chip_id;

	for (uint32_t i = 0; i < signal.count; ++i) {
		if (pool->signals_writers[signal.start + i] & dep_mask) {
			// clear the correspondig bit
			pool->signals_writers[signal.start + i] &= ~dep_mask;

			// prod the dormant writers, or set to default if no writers left
			if (pool->signals_writers[signal.start + i] != 0) {
				pool->rerun_chips |= pool->signals_writers[signal.start + i];
			} else {
				pool->signals_next[signal.start + i] = pool->signals_default[signal.start + i];
				arrpush(pool->signals_written, signal.start + i);
			}
		}
	}
}

static inline bool signal_read_next_bool(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count == 1);

	return pool->signals_next[signal.start];
}

static inline uint8_t signal_read_next_uint8(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 8);

	return signal_read_uint8_internal(pool->signals_next + signal.start, signal.count);
}

static inline uint16_t signal_read_next_uint16(SignalPool *pool, Signal signal) {
	assert(pool);
	assert(signal.count <= 16);

	return signal_read_uint16_internal(pool->signals_next + signal.start, signal.count);
}

static inline bool signal_changed(SignalPool *pool, Signal signal) {
	assert(pool);

	bool result = false;

	for (uint32_t i = 0; i < signal.count; ++i) {
		result |= pool->signals_changed[signal.start + i];
	}

	return result;
}

typedef Signal *SignalGroup;

static inline SignalGroup signal_group_create(void) {
	return NULL;
}

static inline void signal_group_push(SignalGroup *group, Signal signal) {
	assert(group);
	arrpush(*group, signal);
}

static inline int32_t signal_group_read(SignalPool* pool, SignalGroup sg) {
	int32_t result = 0;

	for (size_t i = 0, n = arrlenu(sg); i < n; ++i) {
		result |= (signal_read_bool(pool, sg[i]) << i);
	}
	return result;
}

static inline int32_t signal_group_read_next(SignalPool* pool, SignalGroup sg) {
	int32_t result = 0;

	for (size_t i = 0, n = arrlenu(sg); i < n; ++i) {
		result |= (signal_read_next_bool(pool, sg[i]) << i);
	}
	return result;
}

static inline void signal_group_clear_writer(SignalPool* pool, SignalGroup sg, int32_t chip_id) {
	for (size_t i = 0; i < arrlenu(sg); ++i) {
		signal_clear_writer(pool, sg[i], chip_id);
	}
}

static inline void signal_group_write(SignalPool* pool, SignalGroup sg, int32_t value, int32_t chip_id) {
	assert(pool);
	assert(arrlen(sg) <= 32);

	for (size_t i = 0, n = arrlenu(sg); i < n; ++i) {
		signal_write_bool(pool, sg[i], value & 1, chip_id);
		value >>= 1;
	}
}

static inline void signal_group_write_masked(SignalPool* pool, SignalGroup sg, int32_t value, uint32_t mask, int32_t chip_id) {
	assert(pool);
	assert(arrlen(sg) <= 32);

	if (mask == 0) {
		return;
	}

	for (size_t i = 0, n = arrlenu(sg); i < n; ++i) {
		if (mask & 1) {
			signal_write_bool(pool, sg[i], value & 1, chip_id);
		}
		value >>= 1;
		mask >>= 1;
	}
}

// macros to make working with signal a little prettier
//	define SIGNAL_POOL and SIGNAL_COLLECTION in your source file

#ifndef SIGNAL_ARRAY_STYLE

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

#define SIGNAL_SET_BOOL(sig,v)		signal_write_bool(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (v), SIGNAL_CHIP_ID)
#define SIGNAL_SET_UINT8(sig,v)		signal_write_uint8(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (v), SIGNAL_CHIP_ID)
#define SIGNAL_SET_UINT16(sig,v)	signal_write_uint16(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (v), SIGNAL_CHIP_ID)

#define SIGNAL_SET_UINT8_MASKED(sig,v,m)	signal_write_uint8_masked(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (v), (m), SIGNAL_CHIP_ID)
#define SIGNAL_SET_UINT16_MASKED(sig,v,m)	signal_write_uint16_masked(SIGNAL_POOL, SIGNAL_COLLECTION.sig, (v), (m), SIGNAL_CHIP_ID)

#define	SIGNAL_NO_WRITE(sig)	signal_clear_writer(SIGNAL_POOL, SIGNAL_COLLECTION.sig, SIGNAL_CHIP_ID)

#else // SIGNAL_ARRAY_STYLE

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL( x, y )

#define SIGNAL_POOL			SIGNAL_OWNER->signal_pool
#define SIGNAL_COLLECTION	SIGNAL_OWNER->signals
#define SIGNAL_CHIP_ID		SIGNAL_OWNER->id

#define SIGNAL_DEFINE(sig)												\
	if (SIGNAL_COLLECTION[(sig)].count == 0) {							\
		SIGNAL_COLLECTION[(sig)] = signal_create(SIGNAL_POOL, 1);		\
	}

#define SIGNAL_DEFINE_DEFAULT(sig,def)										\
	if (SIGNAL_COLLECTION[(sig)].count == 0) {								\
		SIGNAL_COLLECTION[(sig)] = signal_create(SIGNAL_POOL, 1);			\
		signal_default_bool(SIGNAL_POOL, SIGNAL_COLLECTION[(sig)], (def));	\
	}

#define SIGNAL(sig)							SIGNAL_COLLECTION[CONCAT(SIGNAL_PREFIX, sig)]

#define SIGNAL_DEPENDENCY(sig)				signal_add_dependency(SIGNAL_POOL, SIGNAL(sig), SIGNAL_CHIP_ID)

#define SIGNAL_READ(sig)					signal_read_bool(SIGNAL_POOL, SIGNAL(sig))
#define SIGNAL_READ_NEXT(sig)				signal_read_next_bool(SIGNAL_POOL, SIGNAL(sig))
#define SIGNAL_CHANGED(sig)					signal_changed(SIGNAL_POOL, SIGNAL(sig))

#define SIGNAL_WRITE(sig,v)					signal_write_bool(SIGNAL_POOL, SIGNAL(sig), (v), SIGNAL_CHIP_ID)

#define	SIGNAL_NO_WRITE(sig)				signal_clear_writer(SIGNAL_POOL, SIGNAL(sig), SIGNAL_CHIP_ID)

#define SIGNAL_GROUP_READ_U8(grp)			((uint8_t) signal_group_read(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp))
#define SIGNAL_GROUP_READ_NEXT_U8(grp)		((uint8_t) signal_group_read_next(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp))
#define SIGNAL_GROUP_WRITE(grp,v)			signal_group_write(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp, (v), SIGNAL_CHIP_ID)
#define SIGNAL_GROUP_WRITE_MASKED(grp,v,m)	signal_group_write_masked(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp, (v), (m), SIGNAL_CHIP_ID)
#define SIGNAL_GROUP_NO_WRITE(grp)			signal_group_clear_writer(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp, SIGNAL_CHIP_ID)

#endif // SIGNAL_ARRAY_STYLE

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIGNAL_LINE_H
