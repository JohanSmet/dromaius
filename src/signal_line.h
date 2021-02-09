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
typedef uint32_t Signal;
typedef Signal *SignalGroup;

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

Signal signal_create(SignalPool *pool);
void signal_set_name(SignalPool *pool, Signal Signal, const char *name);
const char * signal_get_name(SignalPool *pool, Signal signal);
void signal_add_dependency(SignalPool *pool, Signal signal, int32_t chip_id);

void signal_default(SignalPool *pool, Signal signal, bool value);
Signal signal_by_name(SignalPool *pool, const char *name);

static inline bool signal_read(SignalPool *pool, Signal signal) {
	assert(pool);

	return pool->signals_curr[signal];
}

static inline void signal_write(SignalPool *pool, Signal signal, bool value, int32_t chip_id) {
	assert(pool);
	pool->signals_next[signal] = value;
	pool->signals_writers[signal] |= 1ull << chip_id;
	arrpush(pool->signals_written, signal);
}

static inline void signal_clear_writer(SignalPool *pool, Signal signal, int32_t chip_id) {
	assert(pool);

	uint64_t dep_mask = 1ull << chip_id;

	if (pool->signals_writers[signal] & dep_mask) {
		// clear the correspondig bit
		pool->signals_writers[signal] &= ~dep_mask;

		// prod the dormant writers, or set to default if no writers left
		if (pool->signals_writers[signal] != 0) {
			pool->rerun_chips |= pool->signals_writers[signal];
		} else {
			pool->signals_next[signal] = pool->signals_default[signal];
			arrpush(pool->signals_written, signal);
		}
	}
}

static inline bool signal_read_next(SignalPool *pool, Signal signal) {
	assert(pool);

	return pool->signals_next[signal];
}

static inline bool signal_changed(SignalPool *pool, Signal signal) {
	assert(pool);

	return pool->signals_changed[signal];
}

static inline SignalGroup signal_group_create(void) {
	return NULL;
}

static inline SignalGroup signal_group_create_from_array(size_t size, Signal *signals) {
	SignalGroup result = NULL;
	for (size_t i = 0; i < size; ++i) {
		arrpush(result, signals[i]);
	}
	return result;
}

static inline SignalGroup signal_group_create_new(SignalPool *pool, size_t size) {
	SignalGroup result = NULL;
	for (size_t i = 0; i < size; ++i) {
		arrpush(result, signal_create(pool));
	}
	return result;
}

void signal_group_set_name(SignalPool *pool, SignalGroup sg, const char *group_name, const char *signal_name, uint32_t start_idx);

static inline size_t signal_group_size(SignalGroup sg) {
	return arrlenu(sg);
}

static inline void signal_group_push(SignalGroup *group, Signal signal) {
	assert(group);
	arrpush(*group, signal);
}

static inline void signal_group_defaults(SignalPool *pool, SignalGroup sg, int32_t value) {
	assert(arrlen(sg) <= 32);

	for (size_t i = 0, n = arrlenu(sg); i < n; ++i) {
		signal_default(pool, sg[i], value & 1);
		value >>= 1;
	}
}

static inline void signal_group_dependency(SignalPool *pool, SignalGroup sg, int32_t chip_id) {
	assert(arrlen(sg) <= 32);

	for (size_t i = 0, n = arrlenu(sg); i < n; ++i) {
		signal_add_dependency(pool, sg[i], chip_id);
	}
}

static inline int32_t signal_group_read(SignalPool* pool, SignalGroup sg) {
	int32_t result = 0;

	for (size_t i = 0, n = arrlenu(sg); i < n; ++i) {
		result |= (signal_read(pool, sg[i]) << i);
	}
	return result;
}

static inline int32_t signal_group_read_next(SignalPool* pool, SignalGroup sg) {
	int32_t result = 0;

	for (size_t i = 0, n = arrlenu(sg); i < n; ++i) {
		result |= (signal_read_next(pool, sg[i]) << i);
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
		signal_write(pool, sg[i], value & 1, chip_id);
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
			signal_write(pool, sg[i], value & 1, chip_id);
		}
		value >>= 1;
		mask >>= 1;
	}
}

static inline bool signal_group_changed(SignalPool *pool, SignalGroup sg) {
	assert(pool);

	bool result = false;

	for (size_t i = 0, n = arrlenu(sg); i < n; ++i) {
		result |= pool->signals_changed[sg[i]];
	}

	return result;
}

// macros to make working with signal a little prettier
//	define SIGNAL_OWNER and SIGNAL_PREFIX in your source file

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL( x, y )

#define SIGNAL_POOL			SIGNAL_OWNER->signal_pool
#define SIGNAL_COLLECTION	SIGNAL_OWNER->signals
#define SIGNAL_CHIP_ID		SIGNAL_OWNER->id

#define SIGNAL_DEFINE(sig)												\
	if (SIGNAL_COLLECTION[(sig)] == 0) {							\
		SIGNAL_COLLECTION[(sig)] = signal_create(SIGNAL_POOL);		\
	}

#define SIGNAL_DEFINE_N(sig,name)										\
	if (SIGNAL_COLLECTION[(sig)] == 0) {							\
		SIGNAL_COLLECTION[(sig)] = signal_create(SIGNAL_POOL);		\
		signal_set_name(SIGNAL_POOL, SIGNAL_COLLECTION[(sig)], (name));	\
	}


#define SIGNAL_DEFINE_GROUP(sig, grp)									\
	if (SIGNAL_COLLECTION[(sig)] == 0) {							\
		SIGNAL_COLLECTION[(sig)] = signal_create(SIGNAL_POOL);		\
	}																	\
	signal_group_push(&SIGNAL_OWNER->sg_ ## grp, SIGNAL_COLLECTION[(sig)]);

#define SIGNAL_DEFINE_DEFAULT(sig,def)										\
	if (SIGNAL_COLLECTION[(sig)] == 0) {								\
		SIGNAL_COLLECTION[(sig)] = signal_create(SIGNAL_POOL);			\
		signal_default(SIGNAL_POOL, SIGNAL_COLLECTION[(sig)], (def));	\
	}

#define SIGNAL_DEFINE_DEFAULT_N(sig,def,name)								\
	if (SIGNAL_COLLECTION[(sig)] == 0) {								\
		SIGNAL_COLLECTION[(sig)] = signal_create(SIGNAL_POOL);			\
		signal_default(SIGNAL_POOL, SIGNAL_COLLECTION[(sig)], (def));	\
		signal_set_name(SIGNAL_POOL, SIGNAL_COLLECTION[(sig)], (name));	\
	}

#define SIGNAL(sig)							SIGNAL_COLLECTION[CONCAT(SIGNAL_PREFIX, sig)]
#define SIGNAL_GROUP(grp)					SIGNAL_OWNER->sg_ ## grp

#define SIGNAL_DEPENDENCY(sig)				signal_add_dependency(SIGNAL_POOL, SIGNAL(sig), SIGNAL_CHIP_ID)

#define SIGNAL_READ(sig)					signal_read(SIGNAL_POOL, SIGNAL(sig))
#define SIGNAL_READ_NEXT(sig)				signal_read_next(SIGNAL_POOL, SIGNAL(sig))
#define SIGNAL_CHANGED(sig)					signal_changed(SIGNAL_POOL, SIGNAL(sig))

#define SIGNAL_WRITE(sig,v)					signal_write(SIGNAL_POOL, SIGNAL(sig), (v), SIGNAL_CHIP_ID)

#define	SIGNAL_NO_WRITE(sig)				signal_clear_writer(SIGNAL_POOL, SIGNAL(sig), SIGNAL_CHIP_ID)

#define SIGNAL_GROUP_NEW(grp,cnt)			SIGNAL_OWNER->sg_ ## grp = signal_group_create_new(SIGNAL_POOL, (cnt))
#define SIGNAL_GROUP_NEW_N(grp,cnt,gn,sn)	SIGNAL_OWNER->sg_ ## grp = signal_group_create_new(SIGNAL_POOL, (cnt));		\
											signal_group_set_name(SIGNAL_POOL, SIGNAL_GROUP(grp), (gn), (sn), 0);

#define SIGNAL_GROUP_DEPENDENCY(grp)		signal_group_dependency(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp, SIGNAL_CHIP_ID)
#define SIGNAL_GROUP_DEFAULTS(grp,v)		signal_group_defaults(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp, (v))
#define SIGNAL_GROUP_READ_U8(grp)			((uint8_t) signal_group_read(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp))
#define SIGNAL_GROUP_READ_U16(grp)			((uint16_t) signal_group_read(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp))
#define SIGNAL_GROUP_READ_U32(grp)			(signal_group_read(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp))
#define SIGNAL_GROUP_READ_NEXT_U8(grp)		((uint8_t) signal_group_read_next(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp))
#define SIGNAL_GROUP_READ_NEXT_U16(grp)		((uint16_t) signal_group_read_next(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp))
#define SIGNAL_GROUP_READ_NEXT_U32(grp)		(signal_group_read_next(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp))
#define SIGNAL_GROUP_WRITE(grp,v)			signal_group_write(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp, (v), SIGNAL_CHIP_ID)
#define SIGNAL_GROUP_WRITE_MASKED(grp,v,m)	signal_group_write_masked(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp, (v), (m), SIGNAL_CHIP_ID)
#define SIGNAL_GROUP_NO_WRITE(grp)			signal_group_clear_writer(SIGNAL_POOL, SIGNAL_OWNER->sg_ ## grp, SIGNAL_CHIP_ID)

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIGNAL_LINE_H
