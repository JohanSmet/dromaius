// test/test_signal.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"

#include "signal_line.h"
#include "stb/stb_ds.h"

static void *signal_setup(const MunitParameter params[], void *user_data) {
	SignalPool *pool = signal_pool_create();
	return pool;
}

static void signal_teardown(void *fixture) {
	signal_pool_destroy((SignalPool *) fixture);
}

static MunitResult test_create(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// pre-condition
	munit_assert_uint32(pool->signals_count, ==, 1);

	// 1-bit signal
	Signal sig1 = signal_create(pool);
	munit_assert_uint32(sig1, ==, 1);
	munit_assert_uint32(pool->signals_count, ==, 2);

	// 8-bit signal
	SignalGroup sg2 = signal_group_create_new(pool, 8);
	munit_assert_uint32(signal_group_size(sg2), ==, 8);
	munit_assert_uint32(pool->signals_count, ==, 10);

	// 16-bit signal
	SignalGroup sg3 = signal_group_create_new(pool, 16);
	munit_assert_uint32(signal_group_size(sg3), ==, 16);
	munit_assert_uint32(pool->signals_count, ==, 26);

	// 12-bit signal
	SignalGroup sg4 = signal_group_create_new(pool, 12);
	munit_assert_uint32(signal_group_size(sg4), ==, 12);
	munit_assert_uint32(pool->signals_count, ==, 38);

    return MUNIT_OK;
}

static MunitResult test_read(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool);
	Signal sig_b = signal_create(pool);
	pool->signals_value[0] = 0x0000000000000002;

	munit_assert_uint32(sig_a, == , 1);
	munit_assert_uint32(sig_b, == , 2);

	// test
	munit_assert_true(signal_read(pool, sig_a));
	munit_assert_false(signal_read(pool, sig_b));

    return MUNIT_OK;
}

static MunitResult test_write(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal prv = signal_create(pool);
	Signal sig = signal_create(pool);
	Signal nxt = signal_create(pool);

	// test pre-condition
	munit_assert_false(signal_read(pool, prv));
	munit_assert_false(signal_read(pool, sig));
	munit_assert_false(signal_read(pool, nxt));

	// test
	signal_write(pool, sig, true, 0);
	munit_assert_uint64(pool->signals_next_value[0][0], ==, 0x0000000000000004);
	munit_assert_uint64(pool->signals_next_mask[0][0], ==, 0x0000000000000004);

	signal_write(pool, sig, false, 0);
	munit_assert_uint64(pool->signals_next_value[0][0], ==, 0x0000000000000000);
	munit_assert_uint64(pool->signals_next_mask[0][0], ==, 0x0000000000000004);

    return MUNIT_OK;
}

static MunitResult test_read_next(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool);

	// test
	signal_write(pool, sig_a, true, 0);
	munit_assert_false(signal_read(pool, sig_a));
	munit_assert_true(signal_read_next(pool, sig_a));

    return MUNIT_OK;
}

static MunitResult test_cycle(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool);
	Signal sig_b = signal_create(pool);
	Signal sig_c = signal_create(pool);

	// write
	signal_write(pool, sig_a, true, 0);
	signal_write(pool, sig_b, false, 0);
	signal_write(pool, sig_c, true, 0);

	munit_assert_false(signal_read(pool, sig_a));
	munit_assert_false(signal_read(pool, sig_b));
	munit_assert_false(signal_read(pool, sig_c));

	// cycle
	signal_pool_cycle(pool);

	munit_assert_true(signal_read(pool, sig_a));
	munit_assert_false(signal_read(pool, sig_b));
	munit_assert_true(signal_read(pool, sig_c));

    return MUNIT_OK;
}

static MunitResult test_default(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool);
	Signal sig_b = signal_create(pool);
	Signal sig_c = signal_create(pool);

	signal_default(pool, sig_a, false);
	signal_default(pool, sig_b, true);
	signal_default(pool, sig_c, false);

	// test
	signal_write(pool, sig_a, true, 0);
	signal_clear_writer(pool, sig_b, 0);
	signal_clear_writer(pool, sig_c, 0);
	signal_pool_cycle(pool);
	munit_assert_true(signal_read(pool, sig_a));
	munit_assert_true(signal_read(pool, sig_b));
	munit_assert_false(signal_read(pool, sig_c));

	signal_clear_writer(pool, sig_a, 0);
	signal_clear_writer(pool, sig_b, 0);
	signal_clear_writer(pool, sig_c, 0);
	signal_pool_cycle(pool);
	munit_assert_false(signal_read(pool, sig_a));
	munit_assert_true(signal_read(pool, sig_b));
	munit_assert_false(signal_read(pool, sig_c));

	return MUNIT_OK;
}

static MunitResult test_clear_writer(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;
	pool->layer_count = 2;

	// setup
	Signal sig_a = signal_create(pool);
	signal_default(pool, sig_a, true);			// pull-up

	// test
	signal_write(pool, sig_a, false, 0);
	signal_write(pool, sig_a, false, 1);
	signal_pool_cycle(pool);
	munit_assert_false(signal_read(pool, sig_a));

	signal_clear_writer(pool, sig_a, 0);
	signal_pool_cycle(pool);
	munit_assert_false(signal_read(pool, sig_a));

	signal_clear_writer(pool, sig_a, 1);
	signal_pool_cycle(pool);
	munit_assert_true(signal_read(pool, sig_a));

	return MUNIT_OK;
}

static MunitResult test_changed(const MunitParameter params[], void* user_data_or_fixture) {

	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	int64_t current_tick = 0;

	// setup
	Signal sig_a = signal_create(pool);
	Signal sig_b = signal_create(pool);

	// initial state
	signal_write(pool, sig_a, true, 0);
	signal_write(pool, sig_b, false, 0);
	signal_pool_cycle(pool);

	// no change
	++current_tick;
	signal_write(pool, sig_a, true, 0);
	signal_write(pool, sig_b, false, 0);
	signal_pool_cycle(pool);
	munit_assert_false(signal_changed(pool, sig_a));
	munit_assert_false(signal_changed(pool, sig_b));

	// change signal-a
	++current_tick;
	signal_write(pool, sig_a, false, 0);
	signal_write(pool, sig_b, false, 0);
	signal_pool_cycle(pool);
	munit_assert_true(signal_changed(pool, sig_a));
	munit_assert_false(signal_changed(pool, sig_b));

	// change signal-b
	++current_tick;
	signal_write(pool, sig_a, false, 0);
	signal_write(pool, sig_b, true, 0);
	signal_pool_cycle(pool);
	munit_assert_false(signal_changed(pool, sig_a));
	munit_assert_true(signal_changed(pool, sig_b));

	return MUNIT_OK;
}

static MunitResult test_dependencies(const MunitParameter params[], void* user_data_or_fixture) {

	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	Signal sig_a = signal_create(pool);
	signal_add_dependency(pool, sig_a, 1);
	signal_add_dependency(pool, sig_a, 3);

	Signal sig_b = signal_create(pool);
	signal_add_dependency(pool, sig_b, 2);

	Signal sig_c = signal_create(pool);
	signal_add_dependency(pool, sig_c, 3);
	signal_add_dependency(pool, sig_c, 4);

	// no write
	uint64_t dirty_chips = signal_pool_cycle(pool);
	munit_assert_uint64(dirty_chips, ==, 0);

	// change signal-a
	signal_write(pool, sig_a, true, 0);
	dirty_chips = signal_pool_cycle(pool);
	munit_assert_uint64(dirty_chips, ==, 0b00001010);

	// change signal-b
	signal_write(pool, sig_b, true, 0);
	dirty_chips = signal_pool_cycle(pool);
	munit_assert_uint64(dirty_chips, ==, 0b00000100);

	// change signal-c
	signal_write(pool, sig_c, true, 0);
	dirty_chips = signal_pool_cycle(pool);
	munit_assert_uint64(dirty_chips, ==, 0b00011000);

	// change signals a & c
	signal_write(pool, sig_a, false, 0);
	signal_write(pool, sig_c, false, 0);
	dirty_chips = signal_pool_cycle(pool);
	munit_assert_uint64(dirty_chips, ==, 0b00011010);

	return MUNIT_OK;
}

static MunitResult test_names(const MunitParameter params[], void* user_data_or_fixture) {

	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// single signal
	Signal sig_one = signal_create(pool);
	munit_assert_string_equal(signal_get_name(pool, sig_one), "");

	signal_set_name(pool, sig_one, "RES");
	munit_assert_string_equal(signal_get_name(pool, sig_one), "RES");

	// signal group
	SignalGroup sg_multi = signal_group_create_new(pool, 8);
	signal_group_set_name(pool, sg_multi, "multi", "DB%.2d", 0);
	munit_assert_string_equal(signal_get_name(pool, sg_multi[0]), "DB00");
	munit_assert_string_equal(signal_get_name(pool, sg_multi[1]), "DB01");
	munit_assert_string_equal(signal_get_name(pool, sg_multi[2]), "DB02");
	munit_assert_string_equal(signal_get_name(pool, sg_multi[3]), "DB03");
	munit_assert_string_equal(signal_get_name(pool, sg_multi[4]), "DB04");
	munit_assert_string_equal(signal_get_name(pool, sg_multi[5]), "DB05");
	munit_assert_string_equal(signal_get_name(pool, sg_multi[6]), "DB06");
	munit_assert_string_equal(signal_get_name(pool, sg_multi[7]), "DB07");

	return MUNIT_OK;
}

static inline void assert_signal_equal(Signal a, Signal b) {
	munit_assert_uint32(a, ==, b);
}

static MunitResult test_fetch_by_name(const MunitParameter params[], void* user_data_or_fixture) {

	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_bit = signal_create(pool);
	SignalGroup sg_byte = signal_group_create_new(pool, 8);
	SignalGroup sg_word = signal_group_create_new(pool, 16);

	signal_set_name(pool, sig_bit, "bit");
	signal_group_set_name(pool, sg_byte, "byte", "byte_%d", 0);
	signal_group_set_name(pool, sg_word, "word", "word_%x", 0);

	// test
	munit_assert_uint32(signal_by_name(pool, "bit"), ==, sig_bit);

	munit_assert_uint32(signal_by_name(pool, "byte_0"), ==, sg_byte[0]);
	munit_assert_uint32(signal_by_name(pool, "byte_1"), ==, sg_byte[1]);
	munit_assert_uint32(signal_by_name(pool, "byte_2"), ==, sg_byte[2]);
	munit_assert_uint32(signal_by_name(pool, "byte_3"), ==, sg_byte[3]);
	munit_assert_uint32(signal_by_name(pool, "byte_4"), ==, sg_byte[4]);
	munit_assert_uint32(signal_by_name(pool, "byte_5"), ==, sg_byte[5]);
	munit_assert_uint32(signal_by_name(pool, "byte_6"), ==, sg_byte[6]);
	munit_assert_uint32(signal_by_name(pool, "byte_7"), ==, sg_byte[7]);

	munit_assert_uint32(signal_by_name(pool, "word_0"), ==, sg_word[0]);
	munit_assert_uint32(signal_by_name(pool, "word_1"), ==, sg_word[1]);
	munit_assert_uint32(signal_by_name(pool, "word_2"), ==, sg_word[2]);
	munit_assert_uint32(signal_by_name(pool, "word_3"), ==, sg_word[3]);
	munit_assert_uint32(signal_by_name(pool, "word_4"), ==, sg_word[4]);
	munit_assert_uint32(signal_by_name(pool, "word_5"), ==, sg_word[5]);
	munit_assert_uint32(signal_by_name(pool, "word_6"), ==, sg_word[6]);
	munit_assert_uint32(signal_by_name(pool, "word_7"), ==, sg_word[7]);
	munit_assert_uint32(signal_by_name(pool, "word_8"), ==, sg_word[8]);
	munit_assert_uint32(signal_by_name(pool, "word_9"), ==, sg_word[9]);
	munit_assert_uint32(signal_by_name(pool, "word_a"), ==, sg_word[10]);
	munit_assert_uint32(signal_by_name(pool, "word_b"), ==, sg_word[11]);
	munit_assert_uint32(signal_by_name(pool, "word_c"), ==, sg_word[12]);
	munit_assert_uint32(signal_by_name(pool, "word_d"), ==, sg_word[13]);
	munit_assert_uint32(signal_by_name(pool, "word_e"), ==, sg_word[14]);
	munit_assert_uint32(signal_by_name(pool, "word_f"), ==, sg_word[15]);

	Signal unknown = signal_by_name(pool, "unknown");
	munit_assert_uint32(unknown, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_signal_group_read(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	SignalGroup sg_a = signal_group_create_new(pool, 16);
	pool->signals_value[0] = 0xaaaa << 1;

	// test
	munit_assert_uint16(signal_group_read(pool, sg_a), ==, 0xaaaa);

    return MUNIT_OK;
}

static MunitResult test_signal_group_write(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal prev = signal_create(pool);
	SignalGroup sg_a = signal_group_create_new(pool, 8);
	Signal next = signal_create(pool);

	// test pre-condition
	munit_assert_uint64(pool->signals_value[0], ==, 0x0000000000000000);

	// test
	signal_group_write(pool, sg_a, 0xff, 0);
	munit_assert_uint64(pool->signals_value[0], ==, 0x0000000000000000);
	munit_assert_uint64(pool->signals_next_value[0][0], ==, 0x00000000000003fc);
	munit_assert_uint64(pool->signals_next_mask[0][0], ==, 0x00000000000003fc);

	signal_group_write(pool, sg_a, 0x0a, 0);
	munit_assert_uint64(pool->signals_value[0], ==, 0x0000000000000000);
	munit_assert_uint64(pool->signals_next_value[0][0], ==, 0x0000000000000028);
	munit_assert_uint64(pool->signals_next_mask[0][0], ==, 0x00000000000003fc);

    return MUNIT_OK;
}

static MunitResult test_signal_group_write_masked(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	SignalGroup sg_a = signal_group_create_new(pool, 16);

	// test
	signal_group_write_masked(pool, sg_a, 0xaaaa, 0xaf05, 0);
	signal_pool_cycle(pool);
	munit_assert_uint16(signal_group_read(pool, sg_a), ==, 0xaa00);

	signal_group_write_masked(pool, sg_a, 0xabcd, 0xff00, 0);
	munit_assert_uint16(signal_group_read_next(pool, sg_a), ==, 0xab00);
	signal_group_write_masked(pool, sg_a, 0xefcd, 0x00f0, 0);
	munit_assert_uint16(signal_group_read_next(pool, sg_a), ==, 0xabc0);
	signal_pool_cycle(pool);
	munit_assert_uint16(signal_group_read(pool, sg_a), ==, 0xabc0);

	return MUNIT_OK;
}

static MunitResult test_signal_group_read_next(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	SignalGroup sg_a = signal_group_create_new(pool, 16);

	// test
	signal_group_write(pool, sg_a, 0xaa55, 0);
	signal_pool_cycle(pool);
	signal_group_write(pool, sg_a, 0x55aa, 0);

	munit_assert_uint16(signal_group_read(pool, sg_a), ==, 0xaa55);
	munit_assert_uint16(signal_group_read_next(pool, sg_a), ==, 0x55aa);

    return MUNIT_OK;
}

static MunitResult test_signal_group_defaults(const MunitParameter params[], void* user_data_or_fixture) {

	SignalPool *pool = (SignalPool *) user_data_or_fixture;


	// setup
	SignalGroup sg_a = signal_group_create_new(pool, 8);
	SignalGroup sg_b = signal_group_create_new(pool, 8);
	signal_group_defaults(pool, sg_a, 0xa5);

	// test
	signal_group_write(pool, sg_a, 0xfa, 0);
	signal_group_write(pool, sg_b, 0x13, 0);
	signal_pool_cycle(pool);
	munit_assert_uint8(signal_group_read(pool, sg_a), ==, 0xfa);
	munit_assert_uint8(signal_group_read(pool, sg_b), ==, 0x13);

	signal_group_clear_writer(pool, sg_a, 0);
	signal_group_clear_writer(pool, sg_b, 0);
	signal_pool_cycle(pool);
	munit_assert_uint8(signal_group_read(pool, sg_a), ==, 0xa5);
	munit_assert_uint8(signal_group_read(pool, sg_b), ==, 0x00);


	return MUNIT_OK;
}

static MunitResult test_signal_group_changed(const MunitParameter params[], void* user_data_or_fixture) {

	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	int64_t current_tick = 0;

	// setup
	SignalGroup sg_a = signal_group_create_new(pool, 8);
	SignalGroup sg_b = signal_group_create_new(pool, 8);

	// initial state
	signal_group_write(pool, sg_a, 0xaa, 0);
	signal_group_write(pool, sg_b, 0x55, 0);
	signal_pool_cycle(pool);

	// no change
	++current_tick;
	signal_group_write(pool, sg_a, 0xaa, 0);
	signal_group_write(pool, sg_b, 0x55, 0);
	signal_pool_cycle(pool);
	munit_assert_false(signal_group_changed(pool, sg_a));
	munit_assert_false(signal_group_changed(pool, sg_b));

	// change signal-a
	++current_tick;
	signal_group_write(pool, sg_a, 0xbb, 0);
	signal_group_write(pool, sg_b, 0x55, 0);
	signal_pool_cycle(pool);
	munit_assert_true(signal_group_changed(pool, sg_a));
	munit_assert_false(signal_group_changed(pool, sg_b));

	// change signal-b
	++current_tick;
	signal_group_write(pool, sg_a, 0xbb, 0);
	signal_group_write(pool, sg_b, 0x44, 0);
	signal_pool_cycle(pool);
	munit_assert_false(signal_group_changed(pool, sg_a));
	munit_assert_true(signal_group_changed(pool, sg_b));

	return MUNIT_OK;
}

MunitTest signal_tests[] = {
    { "/create", test_create, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read", test_read, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/write", test_write, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read_next", test_read_next, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/cycle", test_cycle, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/default", test_default, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/clear_writer", test_clear_writer, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
	{ "/changed", test_changed, signal_setup, signal_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/dependencies", test_dependencies, signal_setup, signal_teardown, MUNIT_TEST_OPTION_NONE, NULL },
    { "/names", test_names, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/fetch_by_name", test_fetch_by_name, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/group_read", test_signal_group_read, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/group_write", test_signal_group_write, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/group_read_next", test_signal_group_read_next, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/group_write_masked", test_signal_group_write_masked, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/group_defaults", test_signal_group_defaults, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/group_changed", test_signal_group_changed, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
