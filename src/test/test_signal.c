// test/test_signal.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"

#include "signal.h"
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
	munit_assert_size(arrlen(pool->signals_curr), ==, 0);
	munit_assert_size(arrlen(pool->signals_next), ==, 0);

	// 1-bit signal
	Signal sig1 = signal_create(pool, 1);
	munit_assert_uint32(sig1.count, ==, 1);
	munit_assert_size(arrlen(pool->signals_curr), ==, 1);
	munit_assert_size(arrlen(pool->signals_next), ==, 1);

	// 8-bit signal 
	Signal sig2 = signal_create(pool, 8);
	munit_assert_uint32(sig2.count, ==, 8);
	munit_assert_size(arrlen(pool->signals_curr), ==, 9);
	munit_assert_size(arrlen(pool->signals_next), ==, 9);

	// 16-bit signal
	Signal sig3 = signal_create(pool, 16);
	munit_assert_uint32(sig3.count, ==, 16);
	munit_assert_size(arrlen(pool->signals_curr), ==, 25);
	munit_assert_size(arrlen(pool->signals_next), ==, 25);

	// 12-bit signal
	Signal sig4 = signal_create(pool, 12);
	munit_assert_uint32(sig4.count, ==, 12);
	munit_assert_size(arrlen(pool->signals_curr), ==, 37);
	munit_assert_size(arrlen(pool->signals_next), ==, 37);

    return MUNIT_OK;
}

static MunitResult test_read_bool(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;
	
	// setup
	Signal sig_a = signal_create(pool, 1);
	Signal sig_b = signal_create(pool, 1);
	pool->signals_curr[0] = true;
	pool->signals_curr[1] = false;

	// test
	munit_assert_true(signal_read_bool(pool, sig_a));
	munit_assert_false(signal_read_bool(pool, sig_b));

    return MUNIT_OK;
}

static MunitResult test_read_uint8(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;
	
	// setup
	Signal sig_a = signal_create(pool, 8);
	for (int i = 0; i < 4; ++i) {
		pool->signals_curr[(i*2)+0] = true;
		pool->signals_curr[(i*2)+1] = false;
	}

	// test
	munit_assert_uint8(signal_read_uint8(pool, sig_a), ==, 0x55);

    return MUNIT_OK;
}

static MunitResult test_read_uint16(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;
	
	// setup
	Signal sig_a = signal_create(pool, 16);
	for (int i = 0; i < 8; ++i) {
		pool->signals_curr[(i*2)+0] = false;
		pool->signals_curr[(i*2)+1] = true;
	}

	// test
	munit_assert_uint16(signal_read_uint16(pool, sig_a), ==, 0xaaaa);

    return MUNIT_OK;
}

static MunitResult test_write_bool(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;
	
	// setup
	Signal sig_a = signal_create(pool, 1);
	Signal sig_b = signal_create(pool, 1);
	Signal sig_c = signal_create(pool, 1);

	// test pre-condition
	munit_assert_false(pool->signals_next[0]);
	munit_assert_false(pool->signals_next[1]);
	munit_assert_false(pool->signals_next[2]);
	munit_assert_false(pool->signals_curr[0]);
	munit_assert_false(pool->signals_curr[1]);
	munit_assert_false(pool->signals_curr[2]);

	// test
	signal_write_bool(pool, sig_b, true);
	munit_assert_false(pool->signals_next[0]);
	munit_assert_true(pool->signals_next[1]);
	munit_assert_false(pool->signals_next[2]);
	munit_assert_false(pool->signals_curr[0]);
	munit_assert_false(pool->signals_curr[1]);
	munit_assert_false(pool->signals_curr[2]);

	signal_write_bool(pool, sig_b, false);
	munit_assert_false(pool->signals_next[0]);
	munit_assert_false(pool->signals_next[1]);
	munit_assert_false(pool->signals_next[2]);
	munit_assert_false(pool->signals_curr[0]);
	munit_assert_false(pool->signals_curr[1]);
	munit_assert_false(pool->signals_curr[2]);

    return MUNIT_OK;
}

static MunitResult test_write_uint8(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;
	
	// setup
	Signal sig_a = signal_create(pool, 1);
	Signal sig_b = signal_create(pool, 8);
	Signal sig_c = signal_create(pool, 1);

	// test pre-condition
	munit_assert_false(pool->signals_next[0]);
	munit_assert_false(pool->signals_next[1]);
	munit_assert_false(pool->signals_next[2]);
	munit_assert_false(pool->signals_next[3]);
	munit_assert_false(pool->signals_next[4]);
	munit_assert_false(pool->signals_next[5]);
	munit_assert_false(pool->signals_next[6]);
	munit_assert_false(pool->signals_next[7]);
	munit_assert_false(pool->signals_next[8]);
	munit_assert_false(pool->signals_next[9]);
	munit_assert_false(pool->signals_curr[0]);
	munit_assert_false(pool->signals_curr[1]);
	munit_assert_false(pool->signals_curr[2]);
	munit_assert_false(pool->signals_curr[3]);
	munit_assert_false(pool->signals_curr[4]);
	munit_assert_false(pool->signals_curr[5]);
	munit_assert_false(pool->signals_curr[6]);
	munit_assert_false(pool->signals_curr[7]);
	munit_assert_false(pool->signals_curr[8]);
	munit_assert_false(pool->signals_curr[9]);

	// test
	signal_write_uint8(pool, sig_b, 0xff);
	munit_assert_false(pool->signals_next[0]);
	munit_assert_true(pool->signals_next[1]);
	munit_assert_true(pool->signals_next[2]);
	munit_assert_true(pool->signals_next[3]);
	munit_assert_true(pool->signals_next[4]);
	munit_assert_true(pool->signals_next[5]);
	munit_assert_true(pool->signals_next[6]);
	munit_assert_true(pool->signals_next[7]);
	munit_assert_true(pool->signals_next[8]);
	munit_assert_false(pool->signals_next[9]);
	munit_assert_false(pool->signals_curr[0]);
	munit_assert_false(pool->signals_curr[1]);
	munit_assert_false(pool->signals_curr[2]);
	munit_assert_false(pool->signals_curr[3]);
	munit_assert_false(pool->signals_curr[4]);
	munit_assert_false(pool->signals_curr[5]);
	munit_assert_false(pool->signals_curr[6]);
	munit_assert_false(pool->signals_curr[7]);
	munit_assert_false(pool->signals_curr[8]);
	munit_assert_false(pool->signals_curr[9]);

	signal_write_uint8(pool, sig_b, 0x0a);
	munit_assert_false(pool->signals_next[0]);
	munit_assert_false(pool->signals_next[1]);
	munit_assert_true(pool->signals_next[2]);
	munit_assert_false(pool->signals_next[3]);
	munit_assert_true(pool->signals_next[4]);
	munit_assert_false(pool->signals_next[5]);
	munit_assert_false(pool->signals_next[6]);
	munit_assert_false(pool->signals_next[7]);
	munit_assert_false(pool->signals_next[8]);
	munit_assert_false(pool->signals_next[9]);
	munit_assert_false(pool->signals_curr[0]);
	munit_assert_false(pool->signals_curr[1]);
	munit_assert_false(pool->signals_curr[2]);
	munit_assert_false(pool->signals_curr[3]);
	munit_assert_false(pool->signals_curr[4]);
	munit_assert_false(pool->signals_curr[5]);
	munit_assert_false(pool->signals_curr[6]);
	munit_assert_false(pool->signals_curr[7]);
	munit_assert_false(pool->signals_curr[8]);
	munit_assert_false(pool->signals_curr[9]);

    return MUNIT_OK;
}

static MunitResult test_write_uint16(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;
	
	// setup
	Signal sig_a = signal_create(pool, 1);
	Signal sig_b = signal_create(pool, 16);
	Signal sig_c = signal_create(pool, 1);

	// test
	signal_write_uint16(pool, sig_b, 0x5a);
	munit_assert_false(pool->signals_next[0]);
	munit_assert_false(pool->signals_next[1]);
	munit_assert_true(pool->signals_next[2]);
	munit_assert_false(pool->signals_next[3]);
	munit_assert_true(pool->signals_next[4]);
	munit_assert_true(pool->signals_next[5]);
	munit_assert_false(pool->signals_next[6]);
	munit_assert_true(pool->signals_next[7]);
	munit_assert_false(pool->signals_next[8]);
	munit_assert_false(pool->signals_next[9]);
	munit_assert_false(pool->signals_next[10]);
	munit_assert_false(pool->signals_next[11]);
	munit_assert_false(pool->signals_next[12]);
	munit_assert_false(pool->signals_next[13]);
	munit_assert_false(pool->signals_next[14]);
	munit_assert_false(pool->signals_next[15]);
	munit_assert_false(pool->signals_next[16]);
	munit_assert_false(pool->signals_next[17]);

    return MUNIT_OK;
}

static MunitResult test_read_next_bool(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool, 1);

	// test
	signal_write_bool(pool, sig_a, true);
	munit_assert_false(signal_read_bool(pool, sig_a));
	munit_assert_true(signal_read_next_bool(pool, sig_a));

    return MUNIT_OK;
}

static MunitResult test_read_next_uint8(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool, 8);

	// test
	signal_write_uint8(pool, sig_a, 0xaa);
	signal_pool_cycle(pool);
	signal_write_uint8(pool, sig_a, 0x55);

	munit_assert_uint8(signal_read_uint8(pool, sig_a), ==, 0xaa);
	munit_assert_uint8(signal_read_next_uint8(pool, sig_a), ==, 0x55);

    return MUNIT_OK;
}

static MunitResult test_read_next_uint16(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool, 16);

	// test
	signal_write_uint16(pool, sig_a, 0xaabb);
	signal_pool_cycle(pool);
	signal_write_uint16(pool, sig_a, 0x5566);

	munit_assert_uint16(signal_read_uint16(pool, sig_a), ==, 0xaabb);
	munit_assert_uint16(signal_read_next_uint16(pool, sig_a), ==, 0x5566);

    return MUNIT_OK;
}

static MunitResult test_cycle(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool, 1);
	Signal sig_b = signal_create(pool, 1);
	Signal sig_c = signal_create(pool, 1);

	// write
	signal_write_bool(pool, sig_a, true);
	signal_write_bool(pool, sig_b, false);
	signal_write_bool(pool, sig_c, true);

	munit_assert_false(signal_read_bool(pool, sig_a));
	munit_assert_false(signal_read_bool(pool, sig_b));
	munit_assert_false(signal_read_bool(pool, sig_c));
	
	// cycle
	signal_pool_cycle(pool);

	munit_assert_true(signal_read_bool(pool, sig_a));
	munit_assert_false(signal_read_bool(pool, sig_b));
	munit_assert_true(signal_read_bool(pool, sig_c));

    return MUNIT_OK;
}

static MunitResult test_split(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_full = signal_create(pool, 16);
	Signal sig_nib0 = signal_split(sig_full, 0, 4);
	Signal sig_nib1 = signal_split(sig_full, 4, 4);
	Signal sig_nib2 = signal_split(sig_full, 8, 4);
	Signal sig_nib3 = signal_split(sig_full, 12, 4);

	signal_write_uint16(pool, sig_full, 0xdeaf);
	signal_pool_cycle(pool);

	munit_assert_uint16(signal_read_uint16(pool, sig_full), ==, 0xdeaf);
	munit_assert_uint8(signal_read_uint8(pool, sig_nib0), ==, 0xf);
	munit_assert_uint8(signal_read_uint8(pool, sig_nib1), ==, 0xa);
	munit_assert_uint8(signal_read_uint8(pool, sig_nib2), ==, 0xe);
	munit_assert_uint8(signal_read_uint8(pool, sig_nib3), ==, 0xd);

	return MUNIT_OK;
}

static MunitResult test_write_uint8_masked(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;
	
	// setup
	Signal sig = signal_create(pool, 8);
	
	signal_write_uint8(pool, sig, 0x55);
	signal_pool_cycle(pool);
	munit_assert_uint8(signal_read_uint8(pool, sig), ==, 0x55);

	// test 
	signal_write_uint8_masked(pool, sig, 0xaa, 0xaf);
	signal_pool_cycle(pool);
	munit_assert_uint8(signal_read_uint8(pool, sig), ==, 0xfa);

	return MUNIT_OK;
}

static MunitResult test_write_uint16_masked(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;
	
	// setup
	Signal sig = signal_create(pool, 16);
	
	signal_write_uint16(pool, sig, 0x5555);
	signal_pool_cycle(pool);
	munit_assert_uint16(signal_read_uint16(pool, sig), ==, 0x5555);

	// test 
	signal_write_uint16_masked(pool, sig, 0xaaaa, 0xaf05);
	signal_pool_cycle(pool);
	munit_assert_uint16(signal_read_uint16(pool, sig), ==, 0xfa50);

	return MUNIT_OK;
}

static MunitResult test_default_bool(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool, 1);
	Signal sig_b = signal_create(pool, 1);
	Signal sig_c = signal_create(pool, 1);

	signal_default_bool(pool, sig_a, false);
	signal_default_bool(pool, sig_b, true);
	signal_default_bool(pool, sig_c, false);

	// test
	signal_write_bool(pool, sig_a, true);
	signal_pool_cycle(pool);

	munit_assert_true(signal_read_bool(pool, sig_a));
	munit_assert_true(signal_read_bool(pool, sig_b));
	munit_assert_false(signal_read_bool(pool, sig_c));

	signal_pool_cycle(pool);
	munit_assert_false(signal_read_bool(pool, sig_a));
	munit_assert_true(signal_read_bool(pool, sig_b));
	munit_assert_false(signal_read_bool(pool, sig_c));

	return MUNIT_OK;
}

static MunitResult test_default_uint8(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool, 8);
	Signal sig_b = signal_create(pool, 8);
	signal_default_uint8(pool, sig_a, 0xa5);

	// test
	signal_write_uint8(pool, sig_a, 0xfa);
	signal_write_uint8(pool, sig_b, 0x13);
	signal_pool_cycle(pool);
	munit_assert_uint8(signal_read_uint8(pool, sig_a), ==, 0xfa);
	munit_assert_uint8(signal_read_uint8(pool, sig_b), ==, 0x13);

	signal_pool_cycle(pool);
	munit_assert_uint8(signal_read_uint8(pool, sig_a), ==, 0xa5);
	munit_assert_uint8(signal_read_uint8(pool, sig_b), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_default_uint16(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool, 16);
	Signal sig_b = signal_create(pool, 16);
	signal_default_uint16(pool, sig_a, 0x5aa5);

	// test
	signal_write_uint16(pool, sig_a, 0xfaaf);
	signal_write_uint16(pool, sig_b, 0x1331);
	signal_pool_cycle(pool);
	munit_assert_uint16(signal_read_uint16(pool, sig_a), ==, 0xfaaf);
	munit_assert_uint16(signal_read_uint16(pool, sig_b), ==, 0x1331);

	signal_pool_cycle(pool);
	munit_assert_uint16(signal_read_uint16(pool, sig_a), ==, 0x5aa5);
	munit_assert_uint16(signal_read_uint16(pool, sig_b), ==, 0x0000);

	return MUNIT_OK;
}

MunitTest signal_tests[] = {
    { "/create", test_create, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read_bool", test_read_bool, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read_uint8", test_read_uint8, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read_uint16", test_read_uint8, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/write_bool", test_write_bool, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/write_uint8", test_write_uint8, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/write_uint16", test_write_uint8, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read_next_bool", test_read_next_bool, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read_next_uint8", test_read_next_uint8, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read_next_uint16", test_read_next_uint16, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/cycle", test_cycle, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/split", test_split, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/write_uint8_masked", test_write_uint8_masked, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/write_uint16_masked", test_write_uint16_masked, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/default_bool", test_default_bool, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/default_uint8", test_default_uint8, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/default_uint16", test_default_uint16, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
