// signal_line.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_SIGNAL_LINE_H
#define DROMAIUS_SIGNAL_LINE_H

#include "signal_types.h"
#include "signal_pool.h"
#include <assert.h>
#include <string.h>
#include <stb/stb_ds.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const uint64_t lut_bit_to_byte[256];

#define MAX_SIGNAL_NAME		8


// functions - signals
Signal signal_create(SignalPool *pool);

void signal_set_name(SignalPool *pool, Signal Signal, const char *name);
const char * signal_get_name(SignalPool *pool, Signal signal);
Signal signal_by_name(SignalPool *pool, const char *name);

void signal_add_dependency(SignalPool *pool, Signal signal, int32_t chip_id);

void signal_default(SignalPool *pool, Signal signal, bool value);

static inline bool signal_read(SignalPool *pool, Signal signal) {
	assert(pool);
	return pool->signals_value[signal];
}

static inline void signal_write(SignalPool *pool, Signal signal, bool value, int32_t chip_id) {
	assert(pool);
	signal_write_log_add(pool->signals_write_log, 1, signal, value, 1ull << chip_id);
}

static inline void signal_clear_writer(SignalPool *pool, Signal signal, int32_t chip_id) {
	assert(pool);

	const uint64_t w_mask = 1ull << chip_id;

	if (pool->signals_writers[signal] & w_mask) {
		signal_write_log_add(pool->signals_highz_log, 0, signal, false, w_mask);
	}
}

bool signal_read_next(SignalPool *pool, Signal signal);

static inline bool signal_changed(SignalPool *pool, Signal signal) {
	assert(pool);
	return pool->signals_changed[signal];
}

// functions - signal groups

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

// macros to make working with signals a little prettier
//	define SIGNAL_OWNER and SIGNAL_PREFIX in your source file

#define SIGNAL_CONCAT_IMPL(x, y) x##y
#define SIGNAL_CONCAT(x, y) SIGNAL_CONCAT_IMPL( x, y )

#define SIGNAL_POOL			SIGNAL_OWNER->signal_pool
#define SIGNAL_COLLECTION	SIGNAL_OWNER->signals
#define SIGNAL_CHIP_ID		SIGNAL_OWNER->id

#define SIGNAL(sig)			SIGNAL_COLLECTION[SIGNAL_CONCAT(SIGNAL_PREFIX, sig)]
#define SIGNAL_GROUP(grp)	SIGNAL_OWNER->sg_ ## grp

#define SIGNAL_DEFINE(sig)							\
	if (SIGNAL(sig) == 0) {							\
		SIGNAL(sig) = signal_create(SIGNAL_POOL);	\
	}

#define SIGNAL_DEFINE_N(sig,name)							\
	if (SIGNAL(sig) == 0) {									\
		SIGNAL(sig) = signal_create(SIGNAL_POOL);			\
		signal_set_name(SIGNAL_POOL, SIGNAL(sig), (name));	\
	}

#define SIGNAL_DEFINE_GROUP(sig, grp)						\
	if (SIGNAL(sig) == 0) {									\
		SIGNAL(sig) = signal_create(SIGNAL_POOL);			\
	}														\
	signal_group_push(&SIGNAL_OWNER->sg_ ## grp, SIGNAL(sig));

#define SIGNAL_DEFINE_DEFAULT(sig,def)						\
	if (SIGNAL(sig) == 0) {									\
		SIGNAL(sig) = signal_create(SIGNAL_POOL);			\
		signal_default(SIGNAL_POOL, SIGNAL(sig), (def));	\
	}

#define SIGNAL_DEFINE_DEFAULT_N(sig,def,name)				\
	if (SIGNAL(sig) == 0) {									\
		SIGNAL(sig) = signal_create(SIGNAL_POOL);			\
		signal_default(SIGNAL_POOL, SIGNAL(sig), (def));	\
		signal_set_name(SIGNAL_POOL, SIGNAL(sig), (name));	\
	}

#define SIGNAL_DEPENDENCY(sig)				signal_add_dependency(SIGNAL_POOL, SIGNAL(sig), SIGNAL_CHIP_ID)

#define SIGNAL_READ(sig)					signal_read(SIGNAL_POOL, SIGNAL(sig))
#define SIGNAL_READ_NEXT(sig)				signal_read_next(SIGNAL_POOL, SIGNAL(sig))
#define SIGNAL_CHANGED(sig)					signal_changed(SIGNAL_POOL, SIGNAL(sig))

#define SIGNAL_WRITE(sig,v)					signal_write(SIGNAL_POOL, SIGNAL(sig), (v), SIGNAL_CHIP_ID)

#define	SIGNAL_NO_WRITE(sig)				signal_clear_writer(SIGNAL_POOL, SIGNAL(sig), SIGNAL_CHIP_ID)

#define SIGNAL_GROUP_NEW(grp,cnt)			SIGNAL_GROUP(grp)  = signal_group_create_new(SIGNAL_POOL, (cnt))
#define SIGNAL_GROUP_NEW_N(grp,cnt,gn,sn)	SIGNAL_GROUP(grp)  = signal_group_create_new(SIGNAL_POOL, (cnt));		\
											signal_group_set_name(SIGNAL_POOL, SIGNAL_GROUP(grp), (gn), (sn), 0);

#define SIGNAL_GROUP_DEPENDENCY(grp)		signal_group_dependency(SIGNAL_POOL, SIGNAL_GROUP(grp) , SIGNAL_CHIP_ID)
#define SIGNAL_GROUP_DEFAULTS(grp,v)		signal_group_defaults(SIGNAL_POOL, SIGNAL_GROUP(grp) , (v))
#define SIGNAL_GROUP_READ_U8(grp)			((uint8_t) signal_group_read(SIGNAL_POOL, SIGNAL_GROUP(grp) ))
#define SIGNAL_GROUP_READ_U16(grp)			((uint16_t) signal_group_read(SIGNAL_POOL, SIGNAL_GROUP(grp) ))
#define SIGNAL_GROUP_READ_U32(grp)			(signal_group_read(SIGNAL_POOL, SIGNAL_GROUP(grp) ))
#define SIGNAL_GROUP_READ_NEXT_U8(grp)		((uint8_t) signal_group_read_next(SIGNAL_POOL, SIGNAL_GROUP(grp) ))
#define SIGNAL_GROUP_READ_NEXT_U16(grp)		((uint16_t) signal_group_read_next(SIGNAL_POOL, SIGNAL_GROUP(grp) ))
#define SIGNAL_GROUP_READ_NEXT_U32(grp)		(signal_group_read_next(SIGNAL_POOL, SIGNAL_GROUP(grp) ))
#define SIGNAL_GROUP_WRITE(grp,v)			signal_group_write(SIGNAL_POOL, SIGNAL_GROUP(grp) , (v), SIGNAL_CHIP_ID)
#define SIGNAL_GROUP_WRITE_MASKED(grp,v,m)	signal_group_write_masked(SIGNAL_POOL, SIGNAL_GROUP(grp) , (v), (m), SIGNAL_CHIP_ID)
#define SIGNAL_GROUP_NO_WRITE(grp)			signal_group_clear_writer(SIGNAL_POOL, SIGNAL_GROUP(grp) , SIGNAL_CHIP_ID)

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIGNAL_LINE_H
