// test/test_signal_history.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"

#include "signal_history.h"
#include "signal_pool.h"

static void *signal_history_setup(const MunitParameter params[], void *user_data) {
	SignalHistory *history = signal_history_create(16, 4, 32, 6125);
	return history;
}

static void signal_history_teardown(void *fixture) {
	signal_history_destroy((SignalHistory *) fixture);
}

static MunitResult test_create(const MunitParameter params[], void* user_data_or_fixture) {
	SignalHistory *history = (SignalHistory *) user_data_or_fixture;

	munit_assert_uint(history->incoming_count, ==, 16);
	munit_assert_not_null(history->incoming);

	munit_assert_size(history->signal_count, ==, 4);
	munit_assert_size(history->sample_count, ==, 32);
	munit_assert_not_null(history->signal_samples_base);
	munit_assert_not_null(history->signal_samples_head);
	munit_assert_not_null(history->signal_samples_tail);
	munit_assert_not_null(history->samples_time);
	munit_assert_not_null(history->samples_value);

	munit_assert_size(history->signal_samples_base[0], ==, 0);
	munit_assert_size(history->signal_samples_base[1], ==, 32);
	munit_assert_size(history->signal_samples_base[2], ==, 64);
	munit_assert_size(history->signal_samples_base[3], ==, 96);

    return MUNIT_OK;
}

static MunitResult test_push_history(const MunitParameter params[], void* user_data_or_fixture) {
	SignalHistory *history = (SignalHistory *) user_data_or_fixture;

	uint64_t sample_values[SIGNAL_BLOCKS] = {0};
	uint64_t sample_changed[SIGNAL_BLOCKS] = {0};

	munit_assert_true(signal_history_add(history, 1, sample_values, sample_changed, false));

    return MUNIT_OK;
}

static MunitResult test_push_limit(const MunitParameter params[], void* user_data_or_fixture) {
	SignalHistory *history = (SignalHistory *) user_data_or_fixture;

	uint64_t sample_values[SIGNAL_BLOCKS] = {0};
	uint64_t sample_changed[SIGNAL_BLOCKS] = {0};

	for (unsigned int i = 0; i < 15; ++i) {
		munit_assert_true(signal_history_add(history, i, sample_values, sample_changed, false));
	}

	munit_assert_false(signal_history_add(history, 16, sample_values, sample_changed, false));

    return MUNIT_OK;
}

static MunitResult test_process_incoming(const MunitParameter params[], void* user_data_or_fixture) {
	SignalHistory *history = (SignalHistory *) user_data_or_fixture;

	uint64_t sample_values[SIGNAL_BLOCKS] = {0};
	uint64_t sample_changed[SIGNAL_BLOCKS] = {0};

	// nothing to be processed
	munit_assert_false(signal_history_process_incoming_single(history));

	// add initial values
	sample_changed[0] = 0b00001111;
	munit_assert_true(signal_history_add(history, 0, sample_values, sample_changed, false));

	// process entry 1
	munit_assert_true(signal_history_process_incoming_single(history));
	munit_assert_int64(history->samples_time[history->signal_samples_base[0]], ==, 0);
	munit_assert_int64(history->samples_time[history->signal_samples_base[1]], ==, 0);
	munit_assert_int64(history->samples_time[history->signal_samples_base[2]], ==, 0);
	munit_assert_int64(history->samples_time[history->signal_samples_base[3]], ==, 0);
	munit_assert_false(history->samples_value[history->signal_samples_base[0]]);
	munit_assert_false(history->samples_value[history->signal_samples_base[1]]);
	munit_assert_false(history->samples_value[history->signal_samples_base[2]]);
	munit_assert_false(history->samples_value[history->signal_samples_base[3]]);

	// add next values
	sample_changed[0] = 0b00000010;
	sample_values[0]  = 0b00000010;
	munit_assert_true(signal_history_add(history, 7, sample_values, sample_changed, false));

	// process entry 2
	munit_assert_true(signal_history_process_incoming_single(history));
	munit_assert_int64(history->samples_time[history->signal_samples_base[0]+0], ==, 0);
	munit_assert_int64(history->samples_time[history->signal_samples_base[1]+0], ==, 0);
	munit_assert_int64(history->samples_time[history->signal_samples_base[1]+1], ==, 7);
	munit_assert_int64(history->samples_time[history->signal_samples_base[2]+0], ==, 0);
	munit_assert_int64(history->samples_time[history->signal_samples_base[3]+0], ==, 0);
	munit_assert_false(history->samples_value[history->signal_samples_base[0]+0]);
	munit_assert_false(history->samples_value[history->signal_samples_base[1]+0]);
	munit_assert_true (history->samples_value[history->signal_samples_base[1]+1]);
	munit_assert_false(history->samples_value[history->signal_samples_base[2]+0]);
	munit_assert_false(history->samples_value[history->signal_samples_base[3]+0]);

	// add next values
	sample_changed[0] = 0b00001101;
	sample_values[0]  = 0b00001101;
	munit_assert_true(signal_history_add(history, 9, sample_values, sample_changed, false));

	// process entry 3
	munit_assert_true(signal_history_process_incoming_single(history));
	munit_assert_int64(history->samples_time[history->signal_samples_base[0]+0], ==, 0);
	munit_assert_int64(history->samples_time[history->signal_samples_base[0]+1], ==, 9);
	munit_assert_int64(history->samples_time[history->signal_samples_base[1]+0], ==, 0);
	munit_assert_int64(history->samples_time[history->signal_samples_base[1]+1], ==, 7);
	munit_assert_int64(history->samples_time[history->signal_samples_base[2]+0], ==, 0);
	munit_assert_int64(history->samples_time[history->signal_samples_base[2]+1], ==, 9);
	munit_assert_int64(history->samples_time[history->signal_samples_base[3]+0], ==, 0);
	munit_assert_int64(history->samples_time[history->signal_samples_base[3]+1], ==, 9);
	munit_assert_false(history->samples_value[history->signal_samples_base[0]+0]);
	munit_assert_true (history->samples_value[history->signal_samples_base[0]+1]);
	munit_assert_false(history->samples_value[history->signal_samples_base[1]+0]);
	munit_assert_true (history->samples_value[history->signal_samples_base[1]+1]);
	munit_assert_false(history->samples_value[history->signal_samples_base[2]+0]);
	munit_assert_true (history->samples_value[history->signal_samples_base[2]+1]);
	munit_assert_false(history->samples_value[history->signal_samples_base[3]+0]);
	munit_assert_true (history->samples_value[history->signal_samples_base[3]+1]);

	munit_assert_false(signal_history_process_incoming_single(history));

    return MUNIT_OK;
}

static MunitResult test_diagram_data(const MunitParameter params[], void* user_data_or_fixture) {
	SignalHistory *history = (SignalHistory *) user_data_or_fixture;

	// prepare data
	uint64_t sample_values[SIGNAL_BLOCKS] = {0};
	uint64_t sample_changed[SIGNAL_BLOCKS] = {0};

	sample_changed[0] = 0b00000010;
	sample_values[0]  = 0b00000010;
	munit_assert_true(signal_history_add(history, 0, sample_values, sample_changed, false));
	munit_assert_true(signal_history_process_incoming_single(history));

	uint64_t toggle = 0b00001101;

	for (size_t idx = 0; idx < 70; ++idx) {
		uint64_t new_value = sample_values[0] ^ toggle;
		toggle = (~toggle) & 0x0f;
		sample_changed[0] = sample_values[0] ^ new_value;
		sample_values[0]  = new_value;

		munit_assert_true(signal_history_add(history, (idx + 1) * 3, sample_values, sample_changed, false));
		munit_assert_true(signal_history_process_incoming_single(history));
	}

	// check diagram
	SignalHistoryDiagramData diagram_data = {
		.time_begin = 80,
		.time_end = 140
	};
	arrpush(diagram_data.signals, ((Signal) {1, 0, 0}));
	arrpush(diagram_data.signals, ((Signal) {2, 0, 0}));

	signal_history_diagram_data(history, &diagram_data);

	munit_assert_size(arrlen(diagram_data.samples_time), ==, 22);
	munit_assert_size(arrlen(diagram_data.samples_value), ==, 22);
	munit_assert_size(diagram_data.signal_start_offsets[0], ==, 0);
	munit_assert_size(diagram_data.signal_start_offsets[1], ==, 11);

	size_t start_0 = diagram_data.signal_start_offsets[0];
	size_t start_1 = diagram_data.signal_start_offsets[1];
	munit_assert_int64(diagram_data.samples_time[start_0], ==, 138);
	munit_assert_false(diagram_data.samples_value[start_0]);
	munit_assert_int64(diagram_data.samples_time[start_0 + 10], ==, 78);
	munit_assert_false(diagram_data.samples_value[start_0 + 10]);
	munit_assert_int64(diagram_data.samples_time[start_1 + 10 ], ==, 75);
	munit_assert_true(diagram_data.samples_value[start_1 + 10 ]);

    return MUNIT_OK;
}

MunitTest signal_history_tests[] = {
    { "/create", test_create, signal_history_setup, signal_history_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/push_history", test_push_history, signal_history_setup, signal_history_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/push_limit", test_push_limit, signal_history_setup, signal_history_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/process_incoming", test_process_incoming, signal_history_setup, signal_history_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/diagram_data", test_diagram_data, signal_history_setup, signal_history_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
