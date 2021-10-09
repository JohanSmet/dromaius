// signal_line.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal_line.h"

#ifdef DMS_SIGNAL_TRACING
	#include "signal_dumptrace.h"
#endif

#include <stb/stb_ds.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define BITS_TO_BYTE(b)		((((uint64_t) b) & 0b00000001) |				\
							((((uint64_t) b) & 0b00000010) << (1*8-1))   |	\
							((((uint64_t) b) & 0b00000100) << (2*8-2))	 |	\
							((((uint64_t) b) & 0b00001000) << (3*8-3))	 |	\
							((((uint64_t) b) & 0b00010000) << (4*8-4))	 |	\
							((((uint64_t) b) & 0b00100000) << (5*8-5))	 |	\
							((((uint64_t) b) & 0b01000000) << (6*8-6))	 |	\
							((((uint64_t) b) & 0b10000000) << (7*8-7)))

const uint64_t lut_bit_to_byte[256] = {
	BITS_TO_BYTE(0ul),		BITS_TO_BYTE(1ul),		BITS_TO_BYTE(2ul),		BITS_TO_BYTE(3ul),		BITS_TO_BYTE(4ul),
	BITS_TO_BYTE(5ul),		BITS_TO_BYTE(6ul),		BITS_TO_BYTE(7ul),		BITS_TO_BYTE(8ul),		BITS_TO_BYTE(9ul),
	BITS_TO_BYTE(10ul),		BITS_TO_BYTE(11ul),		BITS_TO_BYTE(12ul),		BITS_TO_BYTE(13ul),		BITS_TO_BYTE(14ul),
	BITS_TO_BYTE(15ul),		BITS_TO_BYTE(16ul),		BITS_TO_BYTE(17ul),		BITS_TO_BYTE(18ul),		BITS_TO_BYTE(19ul),
	BITS_TO_BYTE(20ul),		BITS_TO_BYTE(21ul),		BITS_TO_BYTE(22ul),		BITS_TO_BYTE(23ul),		BITS_TO_BYTE(24ul),
	BITS_TO_BYTE(25ul),		BITS_TO_BYTE(26ul),		BITS_TO_BYTE(27ul),		BITS_TO_BYTE(28ul),		BITS_TO_BYTE(29ul),
	BITS_TO_BYTE(30ul),		BITS_TO_BYTE(31ul),		BITS_TO_BYTE(32ul),		BITS_TO_BYTE(33ul),		BITS_TO_BYTE(34ul),
	BITS_TO_BYTE(35ul),		BITS_TO_BYTE(36ul),		BITS_TO_BYTE(37ul),		BITS_TO_BYTE(38ul),		BITS_TO_BYTE(39ul),
	BITS_TO_BYTE(40ul),		BITS_TO_BYTE(41ul),		BITS_TO_BYTE(42ul),		BITS_TO_BYTE(43ul),		BITS_TO_BYTE(44ul),
	BITS_TO_BYTE(45ul),		BITS_TO_BYTE(46ul),		BITS_TO_BYTE(47ul),		BITS_TO_BYTE(48ul),		BITS_TO_BYTE(49ul),
	BITS_TO_BYTE(50ul),		BITS_TO_BYTE(51ul),		BITS_TO_BYTE(52ul),		BITS_TO_BYTE(53ul),		BITS_TO_BYTE(54ul),
	BITS_TO_BYTE(55ul),		BITS_TO_BYTE(56ul),		BITS_TO_BYTE(57ul),		BITS_TO_BYTE(58ul),		BITS_TO_BYTE(59ul),
	BITS_TO_BYTE(60ul),		BITS_TO_BYTE(61ul),		BITS_TO_BYTE(62ul),		BITS_TO_BYTE(63ul),		BITS_TO_BYTE(64ul),
	BITS_TO_BYTE(65ul),		BITS_TO_BYTE(66ul),		BITS_TO_BYTE(67ul),		BITS_TO_BYTE(68ul),		BITS_TO_BYTE(69ul),
	BITS_TO_BYTE(70ul),		BITS_TO_BYTE(71ul),		BITS_TO_BYTE(72ul),		BITS_TO_BYTE(73ul),		BITS_TO_BYTE(74ul),
	BITS_TO_BYTE(75ul),		BITS_TO_BYTE(76ul),		BITS_TO_BYTE(77ul),		BITS_TO_BYTE(78ul),		BITS_TO_BYTE(79ul),
	BITS_TO_BYTE(80ul),		BITS_TO_BYTE(81ul),		BITS_TO_BYTE(82ul),		BITS_TO_BYTE(83ul),		BITS_TO_BYTE(84ul),
	BITS_TO_BYTE(85ul),		BITS_TO_BYTE(86ul),		BITS_TO_BYTE(87ul),		BITS_TO_BYTE(88ul),		BITS_TO_BYTE(89ul),
	BITS_TO_BYTE(90ul),		BITS_TO_BYTE(91ul),		BITS_TO_BYTE(92ul),		BITS_TO_BYTE(93ul),		BITS_TO_BYTE(94ul),
	BITS_TO_BYTE(95ul),		BITS_TO_BYTE(96ul),		BITS_TO_BYTE(97ul),		BITS_TO_BYTE(98ul),		BITS_TO_BYTE(99ul),

	BITS_TO_BYTE(100ul),	BITS_TO_BYTE(101ul),	BITS_TO_BYTE(102ul),	BITS_TO_BYTE(103ul),	BITS_TO_BYTE(104ul),
	BITS_TO_BYTE(105ul),	BITS_TO_BYTE(106ul),	BITS_TO_BYTE(107ul),	BITS_TO_BYTE(108ul),	BITS_TO_BYTE(109ul),
	BITS_TO_BYTE(110ul),	BITS_TO_BYTE(111ul),	BITS_TO_BYTE(112ul),	BITS_TO_BYTE(113ul),	BITS_TO_BYTE(114ul),
	BITS_TO_BYTE(115ul),	BITS_TO_BYTE(116ul),	BITS_TO_BYTE(117ul),	BITS_TO_BYTE(118ul),	BITS_TO_BYTE(119ul),
	BITS_TO_BYTE(120ul),	BITS_TO_BYTE(121ul),	BITS_TO_BYTE(122ul),	BITS_TO_BYTE(123ul),	BITS_TO_BYTE(124ul),
	BITS_TO_BYTE(125ul),	BITS_TO_BYTE(126ul),	BITS_TO_BYTE(127ul),	BITS_TO_BYTE(128ul),	BITS_TO_BYTE(129ul),
	BITS_TO_BYTE(130ul),	BITS_TO_BYTE(131ul),	BITS_TO_BYTE(132ul),	BITS_TO_BYTE(133ul),	BITS_TO_BYTE(134ul),
	BITS_TO_BYTE(135ul),	BITS_TO_BYTE(136ul),	BITS_TO_BYTE(137ul),	BITS_TO_BYTE(138ul),	BITS_TO_BYTE(139ul),
	BITS_TO_BYTE(140ul),	BITS_TO_BYTE(141ul),	BITS_TO_BYTE(142ul),	BITS_TO_BYTE(143ul),	BITS_TO_BYTE(144ul),
	BITS_TO_BYTE(145ul),	BITS_TO_BYTE(146ul),	BITS_TO_BYTE(147ul),	BITS_TO_BYTE(148ul),	BITS_TO_BYTE(149ul),
	BITS_TO_BYTE(150ul),	BITS_TO_BYTE(151ul),	BITS_TO_BYTE(152ul),	BITS_TO_BYTE(153ul),	BITS_TO_BYTE(154ul),
	BITS_TO_BYTE(155ul),	BITS_TO_BYTE(156ul),	BITS_TO_BYTE(157ul),	BITS_TO_BYTE(158ul),	BITS_TO_BYTE(159ul),
	BITS_TO_BYTE(160ul),	BITS_TO_BYTE(161ul),	BITS_TO_BYTE(162ul),	BITS_TO_BYTE(163ul),	BITS_TO_BYTE(164ul),
	BITS_TO_BYTE(165ul),	BITS_TO_BYTE(166ul),	BITS_TO_BYTE(167ul),	BITS_TO_BYTE(168ul),	BITS_TO_BYTE(169ul),
	BITS_TO_BYTE(170ul),	BITS_TO_BYTE(171ul),	BITS_TO_BYTE(172ul),	BITS_TO_BYTE(173ul),	BITS_TO_BYTE(174ul),
	BITS_TO_BYTE(175ul),	BITS_TO_BYTE(176ul),	BITS_TO_BYTE(177ul),	BITS_TO_BYTE(178ul),	BITS_TO_BYTE(179ul),
	BITS_TO_BYTE(180ul),	BITS_TO_BYTE(181ul),	BITS_TO_BYTE(182ul),	BITS_TO_BYTE(183ul),	BITS_TO_BYTE(184ul),
	BITS_TO_BYTE(185ul),	BITS_TO_BYTE(186ul),	BITS_TO_BYTE(187ul),	BITS_TO_BYTE(188ul),	BITS_TO_BYTE(189ul),
	BITS_TO_BYTE(190ul),	BITS_TO_BYTE(191ul),	BITS_TO_BYTE(192ul),	BITS_TO_BYTE(193ul),	BITS_TO_BYTE(194ul),
	BITS_TO_BYTE(195ul),	BITS_TO_BYTE(196ul),	BITS_TO_BYTE(197ul),	BITS_TO_BYTE(198ul),	BITS_TO_BYTE(199ul),

	BITS_TO_BYTE(200ul),	BITS_TO_BYTE(201ul),	BITS_TO_BYTE(202ul),	BITS_TO_BYTE(203ul),	BITS_TO_BYTE(204ul),
	BITS_TO_BYTE(205ul),	BITS_TO_BYTE(206ul),	BITS_TO_BYTE(207ul),	BITS_TO_BYTE(208ul),	BITS_TO_BYTE(209ul),
	BITS_TO_BYTE(210ul),	BITS_TO_BYTE(211ul),	BITS_TO_BYTE(212ul),	BITS_TO_BYTE(213ul),	BITS_TO_BYTE(214ul),
	BITS_TO_BYTE(215ul),	BITS_TO_BYTE(216ul),	BITS_TO_BYTE(217ul),	BITS_TO_BYTE(218ul),	BITS_TO_BYTE(219ul),
	BITS_TO_BYTE(220ul),	BITS_TO_BYTE(221ul),	BITS_TO_BYTE(222ul),	BITS_TO_BYTE(223ul),	BITS_TO_BYTE(224ul),
	BITS_TO_BYTE(225ul),	BITS_TO_BYTE(226ul),	BITS_TO_BYTE(227ul),	BITS_TO_BYTE(228ul),	BITS_TO_BYTE(229ul),
	BITS_TO_BYTE(230ul),	BITS_TO_BYTE(231ul),	BITS_TO_BYTE(232ul),	BITS_TO_BYTE(233ul),	BITS_TO_BYTE(234ul),
	BITS_TO_BYTE(235ul),	BITS_TO_BYTE(236ul),	BITS_TO_BYTE(237ul),	BITS_TO_BYTE(238ul),	BITS_TO_BYTE(239ul),
	BITS_TO_BYTE(240ul),	BITS_TO_BYTE(241ul),	BITS_TO_BYTE(242ul),	BITS_TO_BYTE(243ul),	BITS_TO_BYTE(244ul),
	BITS_TO_BYTE(245ul),	BITS_TO_BYTE(246ul),	BITS_TO_BYTE(247ul),	BITS_TO_BYTE(248ul),	BITS_TO_BYTE(249ul),
	BITS_TO_BYTE(250ul),	BITS_TO_BYTE(251ul),	BITS_TO_BYTE(252ul),	BITS_TO_BYTE(253ul),	BITS_TO_BYTE(254ul),
	BITS_TO_BYTE(255ul)
};

SignalPool *signal_pool_create(size_t max_concurrent_writes) {
	SignalPool *pool = (SignalPool *) calloc(1, sizeof(SignalPool));
	sh_new_arena(pool->signal_names);

	// NULL-signal: to detect uninitialized signals
	signal_create(pool);

	// create queues
	pool->signals_write_queue[0].queue = (SignalQueueEntry *) malloc(max_concurrent_writes * sizeof(SignalQueueEntry));
	pool->signals_write_queue[1].queue = (SignalQueueEntry *) malloc(max_concurrent_writes * sizeof(SignalQueueEntry));
	pool->signals_highz_queue.queue = (SignalQueueEntry *) malloc(max_concurrent_writes * sizeof(SignalQueueEntry));

	return pool;
}

void signal_pool_destroy(SignalPool *pool) {
	assert(pool);

	arrfree(pool->signals_value);
	arrfree(pool->signals_writers);
	arrfree(pool->signals_next_value);
	arrfree(pool->signals_next_writers);
	arrfree(pool->signals_default);
	arrfree(pool->signals_name);
	arrfree(pool->signals_changed);
	arrfree(pool->dependent_components);

	free(pool->signals_write_queue[0].queue);
	free(pool->signals_write_queue[1].queue);
	free(pool->signals_highz_queue.queue);

	free(pool);
}

static inline uint64_t signal_pool_process_high_impedance__internal(SignalPool *pool) {
	assert(pool);

	uint64_t rerun_chips = 0;

	for (size_t i = 0, n = pool->signals_highz_queue.size; i < n; ++i) {
		const Signal s = pool->signals_highz_queue.queue[i].signal;
		const uint64_t w_mask = pool->signals_highz_queue.queue[i].writer_mask;

		// clear the corresponding bit in the writers mask
		pool->signals_writers[s] &= ~w_mask;

		if (pool->signals_writers[s] != 0) {
			rerun_chips |= pool->signals_writers[s];
		} else {
			// TODO: replace with signal_write call that has a queue-index parameter
			SignalQueueEntry *entry = &pool->signals_write_queue[0].queue[pool->signals_write_queue[0].size++];
			entry->signal = s;
			entry->writer_mask = 0;
			entry->new_value  = pool->signals_default[s];
		}
	}

	// clearn highz queue
	pool->signals_highz_queue.size = 0;

	return rerun_chips;
}

uint64_t signal_pool_process_high_impedance(SignalPool *pool) {
	return signal_pool_process_high_impedance__internal(pool);
}

uint64_t signal_pool_cycle(SignalPool *pool) {

	signal_pool_process_high_impedance__internal(pool);

	// handle writes
	memset(pool->signals_changed, false, arrlenu(pool->signals_changed));
	uint64_t dirty_chips = 0;

	for (int q = 0; q < 2; ++q) {
		for (size_t i = 0, n = pool->signals_write_queue[q].size; i < n; ++i) {
			const SignalQueueEntry *sw = &pool->signals_write_queue[q].queue[i];
			pool->signals_changed[sw->signal] = pool->signals_value[sw->signal] != sw->new_value;
		}
	}

	for (int q = 0; q < 2; ++q) {
		for (size_t i = 0, n = pool->signals_write_queue[q].size; i < n; ++i) {
			const SignalQueueEntry *sw = &pool->signals_write_queue[q].queue[i];

			pool->signals_writers[sw->signal] |= sw->writer_mask;
			pool->signals_value[sw->signal] = sw->new_value;

			const uint64_t mask = (uint64_t) (!pool->signals_changed[sw->signal] - 1);
			dirty_chips |= mask & pool->dependent_components[sw->signal];
		}

		pool->signals_write_queue[q].size = 0;
	}

	pool->swq_snapshot = 0;
	pool->cycle_count += 1;

	return dirty_chips;
}

Signal signal_create(SignalPool *pool) {
	assert(pool);

	Signal result = (uint32_t) arrlenu(pool->signals_value);

	arrpush(pool->signals_changed, false);
	arrpush(pool->signals_value, false);
	arrpush(pool->signals_next_value, 0);
	arrpush(pool->signals_default, false);
	arrpush(pool->signals_name, NULL);
	arrpush(pool->signals_writers, 0);
	arrpush(pool->signals_next_writers, 0);
	arrpush(pool->dependent_components, 0);

	return result;
}

void signal_set_name(SignalPool *pool, Signal signal, const char *name) {
	assert(pool);
	assert(name);

	shput(pool->signal_names, name, signal);
	pool->signals_name[signal] = pool->signal_names[shlenu(pool->signal_names) - 1].key;
}

const char *signal_get_name(SignalPool *pool, Signal signal) {
	assert(pool);

	const char *name = pool->signals_name[signal];
	return (name == NULL) ? "" : name;
}

void signal_add_dependency(SignalPool *pool, Signal signal, int32_t chip_id) {
	assert(pool);
	assert(chip_id >= 0 && chip_id < 64);

	uint64_t dep_mask = 1ull << chip_id;
	pool->dependent_components[signal] |= dep_mask;
}

void signal_default(SignalPool *pool, Signal signal, bool value) {
	assert(pool);

	pool->signals_default[signal] = value;
	pool->signals_value[signal] = value;
}

Signal signal_by_name(SignalPool *pool, const char *name) {
	return shget(pool->signal_names, name);
}

bool signal_read_next(SignalPool *pool, Signal signal) {
	assert(pool);

	if (pool->read_next_cycle != pool->cycle_count ||
		pool->swq_snapshot != pool->signals_write_queue[1].size + pool->signals_highz_queue.size) {

		memcpy(pool->signals_next_value, pool->signals_value, sizeof(*pool->signals_value) * arrlenu(pool->signals_value));
		memcpy(pool->signals_next_writers, pool->signals_writers, sizeof(*pool->signals_writers) * arrlenu(pool->signals_writers));

		for (size_t i = 0, n = pool->signals_highz_queue.size; i < n; ++i) {
			const Signal s = pool->signals_highz_queue.queue[i].signal;
			const uint64_t w_mask = pool->signals_highz_queue.queue[i].writer_mask;

			// clear the corresponding bit in the writers mask
			pool->signals_next_writers[s] &= ~w_mask;

			if (pool->signals_next_writers[s] == 0) {
				pool->signals_next_value[s] =  pool->signals_default[s];
			}
		}

		for (size_t i = 0, n = pool->signals_write_queue[1].size; i < n; ++i) {
			SignalQueueEntry *sw = &pool->signals_write_queue[1].queue[i];
			pool->signals_next_value[sw->signal] = sw->new_value;
		}

		pool->swq_snapshot = pool->signals_write_queue[1].size + pool->signals_highz_queue.size;
		pool->read_next_cycle = pool->cycle_count;
	}

	return pool->signals_next_value[signal];
}

void signal_group_set_name(SignalPool *pool, SignalGroup sg, const char *group_name, const char *signal_name, uint32_t start_idx) {
	assert(pool);
	assert(sg);
	assert(group_name);
	assert(signal_name);

	// TODO: store group name
	(void) group_name;

	// name individual signals
	char sub_name[MAX_SIGNAL_NAME];
	for (uint32_t i = 0; i < signal_group_size(sg); ++i) {
		snprintf(sub_name, MAX_SIGNAL_NAME, signal_name, i + start_idx);
		shput(pool->signal_names, sub_name, sg[i]);
		pool->signals_name[sg[i]] = pool->signal_names[shlenu(pool->signal_names) - 1].key;
	}
}
