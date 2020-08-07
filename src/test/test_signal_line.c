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
	signal_create(pool, 1);
	Signal sig = signal_create(pool, 1);
	signal_create(pool, 1);

	// test pre-condition
	munit_assert_false(pool->signals_next[0]);
	munit_assert_false(pool->signals_next[1]);
	munit_assert_false(pool->signals_next[2]);
	munit_assert_false(pool->signals_curr[0]);
	munit_assert_false(pool->signals_curr[1]);
	munit_assert_false(pool->signals_curr[2]);

	// test
	signal_write_bool(pool, sig, true, 0);
	munit_assert_false(pool->signals_next[0]);
	munit_assert_true(pool->signals_next[1]);
	munit_assert_false(pool->signals_next[2]);
	munit_assert_false(pool->signals_curr[0]);
	munit_assert_false(pool->signals_curr[1]);
	munit_assert_false(pool->signals_curr[2]);

	signal_write_bool(pool, sig, false, 0);
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
	signal_create(pool, 1);
	Signal sig = signal_create(pool, 8);
	signal_create(pool, 1);

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
	signal_write_uint8(pool, sig, 0xff, 0);
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

	signal_write_uint8(pool, sig, 0x0a, 0);
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
	signal_create(pool, 1);
	Signal sig = signal_create(pool, 16);
	signal_create(pool, 1);

	// test
	signal_write_uint16(pool, sig, 0x5a, 0);
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
	signal_write_bool(pool, sig_a, true, 0);
	munit_assert_false(signal_read_bool(pool, sig_a));
	munit_assert_true(signal_read_next_bool(pool, sig_a));

    return MUNIT_OK;
}

static MunitResult test_read_next_uint8(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool, 8);

	// test
	signal_write_uint8(pool, sig_a, 0xaa, 0);
	signal_pool_cycle(pool);
	signal_write_uint8(pool, sig_a, 0x55, 0);

	munit_assert_uint8(signal_read_uint8(pool, sig_a), ==, 0xaa);
	munit_assert_uint8(signal_read_next_uint8(pool, sig_a), ==, 0x55);

    return MUNIT_OK;
}

static MunitResult test_read_next_uint16(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool, 16);

	// test
	signal_write_uint16(pool, sig_a, 0xaabb, 0);
	signal_pool_cycle(pool);
	signal_write_uint16(pool, sig_a, 0x5566, 0);

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
	signal_write_bool(pool, sig_a, true, 0);
	signal_write_bool(pool, sig_b, false, 0);
	signal_write_bool(pool, sig_c, true, 0);

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

	signal_write_uint16(pool, sig_full, 0xdeaf, 0);
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

	// test
	signal_write_uint8_masked(pool, sig, 0x5a, 0xaf, 0);
	signal_pool_cycle(pool);
	munit_assert_uint8(signal_read_uint8(pool, sig), ==, 0x0a);

	signal_write_uint8_masked(pool, sig, 0xde, 0xf0, 0);
	signal_write_uint8_masked(pool, sig, 0xed, 0x0f, 0);
	signal_pool_cycle(pool);
	munit_assert_uint8(signal_read_uint8(pool, sig), ==, 0xdd);

	return MUNIT_OK;
}

static MunitResult test_write_uint16_masked(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig = signal_create(pool, 16);

	// test
	signal_write_uint16_masked(pool, sig, 0xaaaa, 0xaf05, 0);
	signal_pool_cycle(pool);
	munit_assert_uint16(signal_read_uint16(pool, sig), ==, 0xaa00);

	signal_write_uint16_masked(pool, sig, 0xabcd, 0xff00, 0);
	munit_assert_uint16(signal_read_next_uint16(pool, sig), ==, 0xab00);
	signal_write_uint16_masked(pool, sig, 0xefcd, 0x00f0, 0);
	munit_assert_uint16(signal_read_next_uint16(pool, sig), ==, 0xabc0);
	signal_pool_cycle(pool);
	munit_assert_uint16(signal_read_uint16(pool, sig), ==, 0xabc0);

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
	signal_write_bool(pool, sig_a, true, 0);
	signal_clear_writer(pool, sig_b, 0);
	signal_clear_writer(pool, sig_c, 0);
	signal_pool_cycle(pool);
	munit_assert_true(signal_read_bool(pool, sig_a));
	munit_assert_true(signal_read_bool(pool, sig_b));
	munit_assert_false(signal_read_bool(pool, sig_c));

	signal_clear_writer(pool, sig_a, 0);
	signal_clear_writer(pool, sig_b, 0);
	signal_clear_writer(pool, sig_c, 0);
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
	signal_write_uint8(pool, sig_a, 0xfa, 0);
	signal_write_uint8(pool, sig_b, 0x13, 0);
	signal_pool_cycle(pool);
	munit_assert_uint8(signal_read_uint8(pool, sig_a), ==, 0xfa);
	munit_assert_uint8(signal_read_uint8(pool, sig_b), ==, 0x13);

	signal_clear_writer(pool, sig_a, 0);
	signal_clear_writer(pool, sig_b, 0);
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
	signal_write_uint16(pool, sig_a, 0xfaaf, 0);
	signal_write_uint16(pool, sig_b, 0x1331, 0);
	signal_pool_cycle(pool);
	munit_assert_uint16(signal_read_uint16(pool, sig_a), ==, 0xfaaf);
	munit_assert_uint16(signal_read_uint16(pool, sig_b), ==, 0x1331);

	signal_clear_writer(pool, sig_a, 0);
	signal_clear_writer(pool, sig_b, 0);
	signal_pool_cycle(pool);
	munit_assert_uint16(signal_read_uint16(pool, sig_a), ==, 0x5aa5);
	munit_assert_uint16(signal_read_uint16(pool, sig_b), ==, 0x0000);

	return MUNIT_OK;
}

static MunitResult test_clear_writer(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_a = signal_create(pool, 1);
	signal_default_bool(pool, sig_a, true);			// pull-up

	// test
	signal_write_bool(pool, sig_a, false, 5);
	signal_write_bool(pool, sig_a, false, 3);
	munit_assert_false(signal_read_next_bool(pool, sig_a));

	signal_clear_writer(pool, sig_a, 5);
	munit_assert_false(signal_read_next_bool(pool, sig_a));

	signal_clear_writer(pool, sig_a, 3);
	munit_assert_true(signal_read_next_bool(pool, sig_a));

	return MUNIT_OK;
}

static MunitResult test_changed(const MunitParameter params[], void* user_data_or_fixture) {

	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_bit = signal_create(pool, 1);
	Signal sig_byte = signal_create(pool, 8);
	Signal sig_word = signal_create(pool, 16);

	// initial state
	signal_write_bool(pool, sig_bit, true, 0);
	signal_write_uint8(pool, sig_byte, 0xaf, 0);
	signal_write_uint16(pool, sig_word, 0xbeef, 0);
	signal_pool_cycle(pool);

	// no change
	++pool->current_tick;
	signal_write_bool(pool, sig_bit, true, 0);
	signal_write_uint8(pool, sig_byte, 0xaf, 0);
	signal_write_uint16(pool, sig_word, 0xbeef, 0);
	signal_pool_cycle(pool);
	munit_assert_false(signal_changed(pool, sig_bit));
	munit_assert_false(signal_changed(pool, sig_byte));
	munit_assert_false(signal_changed(pool, sig_word));

	// change bit
	++pool->current_tick;
	signal_write_bool(pool, sig_bit, false, 0);
	signal_write_uint8(pool, sig_byte, 0xaf, 0);
	signal_write_uint16(pool, sig_word, 0xbeef, 0);
	signal_pool_cycle(pool);
	munit_assert_true(signal_changed(pool, sig_bit));
	munit_assert_false(signal_changed(pool, sig_byte));
	munit_assert_false(signal_changed(pool, sig_word));

	// change byte
	++pool->current_tick;
	signal_write_bool(pool, sig_bit, false, 0);
	signal_write_uint8(pool, sig_byte, 0xfa, 0);
	signal_write_uint16(pool, sig_word, 0xbeef, 0);
	signal_pool_cycle(pool);
	munit_assert_false(signal_changed(pool, sig_bit));
	munit_assert_true(signal_changed(pool, sig_byte));
	munit_assert_false(signal_changed(pool, sig_word));

	// change word
	++pool->current_tick;
	signal_write_bool(pool, sig_bit, false, 0);
	signal_write_uint8(pool, sig_byte, 0xfa, 0);
	signal_write_uint16(pool, sig_word, 0xdead, 0);
	signal_pool_cycle(pool);
	munit_assert_false(signal_changed(pool, sig_bit));
	munit_assert_false(signal_changed(pool, sig_byte));
	munit_assert_true(signal_changed(pool, sig_word));

	return MUNIT_OK;
}

static MunitResult test_names(const MunitParameter params[], void* user_data_or_fixture) {

	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// 1-bit
	Signal sig_one = signal_create(pool, 1);
	munit_assert_string_equal(signal_get_name(pool, sig_one), "");

	signal_set_name(pool, sig_one, "RES");
	munit_assert_string_equal(signal_get_name(pool, sig_one), "RES");

	// multi-bit
	Signal sig_multi = signal_create(pool, 8);
	signal_set_name(pool, sig_multi, "DB%.2d");
	munit_assert_string_equal(signal_get_name(pool, signal_split(sig_multi, 0, 1)), "DB00");
	munit_assert_string_equal(signal_get_name(pool, signal_split(sig_multi, 1, 1)), "DB01");
	munit_assert_string_equal(signal_get_name(pool, signal_split(sig_multi, 2, 1)), "DB02");
	munit_assert_string_equal(signal_get_name(pool, signal_split(sig_multi, 3, 1)), "DB03");
	munit_assert_string_equal(signal_get_name(pool, signal_split(sig_multi, 4, 1)), "DB04");
	munit_assert_string_equal(signal_get_name(pool, signal_split(sig_multi, 5, 1)), "DB05");
	munit_assert_string_equal(signal_get_name(pool, signal_split(sig_multi, 6, 1)), "DB06");
	munit_assert_string_equal(signal_get_name(pool, signal_split(sig_multi, 7, 1)), "DB07");

	return MUNIT_OK;
}

static inline void assert_signal_equal(Signal a, Signal b) {
	munit_assert_uint32(a.start, ==, b.start);
	munit_assert_uint32(a.count, ==, b.count);
}

static MunitResult test_fetch_by_name(const MunitParameter params[], void* user_data_or_fixture) {

	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	// setup
	Signal sig_bit = signal_create(pool, 1);
	Signal sig_byte = signal_create(pool, 8);
	Signal sig_word = signal_create(pool, 16);

	signal_set_name(pool, sig_bit, "bit");
	signal_set_name(pool, sig_byte, "byte_%d");
	signal_set_name(pool, sig_word, "word_%x");

	// test
	assert_signal_equal(signal_by_name(pool, "bit"), sig_bit);

	assert_signal_equal(signal_by_name(pool, "byte_0"), signal_split(sig_byte, 0, 1));
	assert_signal_equal(signal_by_name(pool, "byte_1"), signal_split(sig_byte, 1, 1));
	assert_signal_equal(signal_by_name(pool, "byte_2"), signal_split(sig_byte, 2, 1));
	assert_signal_equal(signal_by_name(pool, "byte_3"), signal_split(sig_byte, 3, 1));
	assert_signal_equal(signal_by_name(pool, "byte_4"), signal_split(sig_byte, 4, 1));
	assert_signal_equal(signal_by_name(pool, "byte_5"), signal_split(sig_byte, 5, 1));
	assert_signal_equal(signal_by_name(pool, "byte_6"), signal_split(sig_byte, 6, 1));
	assert_signal_equal(signal_by_name(pool, "byte_7"), signal_split(sig_byte, 7, 1));

	assert_signal_equal(signal_by_name(pool, "word_0"), signal_split(sig_word, 0, 1));
	assert_signal_equal(signal_by_name(pool, "word_1"), signal_split(sig_word, 1, 1));
	assert_signal_equal(signal_by_name(pool, "word_2"), signal_split(sig_word, 2, 1));
	assert_signal_equal(signal_by_name(pool, "word_3"), signal_split(sig_word, 3, 1));
	assert_signal_equal(signal_by_name(pool, "word_4"), signal_split(sig_word, 4, 1));
	assert_signal_equal(signal_by_name(pool, "word_5"), signal_split(sig_word, 5, 1));
	assert_signal_equal(signal_by_name(pool, "word_6"), signal_split(sig_word, 6, 1));
	assert_signal_equal(signal_by_name(pool, "word_7"), signal_split(sig_word, 7, 1));
	assert_signal_equal(signal_by_name(pool, "word_8"), signal_split(sig_word, 8, 1));
	assert_signal_equal(signal_by_name(pool, "word_9"), signal_split(sig_word, 9, 1));
	assert_signal_equal(signal_by_name(pool, "word_a"), signal_split(sig_word, 10, 1));
	assert_signal_equal(signal_by_name(pool, "word_b"), signal_split(sig_word, 11, 1));
	assert_signal_equal(signal_by_name(pool, "word_c"), signal_split(sig_word, 12, 1));
	assert_signal_equal(signal_by_name(pool, "word_d"), signal_split(sig_word, 13, 1));
	assert_signal_equal(signal_by_name(pool, "word_e"), signal_split(sig_word, 14, 1));
	assert_signal_equal(signal_by_name(pool, "word_f"), signal_split(sig_word, 15, 1));

	Signal unknown = signal_by_name(pool, "unknown");
	munit_assert_uint32(unknown.count, ==, 0);

	return MUNIT_OK;
}

static MunitResult benchmark_read_uint8(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	Signal *signals = (Signal *) malloc(sizeof(Signal) * 1024);
	for (int i = 0; i < 1024; ++i) {
		signals[i] = signal_create(pool, 8);
	}

	for (int j = 0; j < 102400; ++j) {
		for (int i = 0; i < 1024; ++i) {
			uint8_t r = signal_read_uint8(pool, signals[i]);
			(void) r;
		}
	}

	free(signals);
	return MUNIT_OK;
}

static MunitResult benchmark_write_uint8(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	Signal *signals = (Signal *) malloc(sizeof(Signal) * 1024);
	for (int i = 0; i < 1024; ++i) {
		signals[i] = signal_create(pool, 8);
	}

	for (int j = 0; j < 102400; ++j) {
		for (int i = 0; i < 1024; ++i) {
			signal_write_uint8(pool, signals[i], j & 255, 0);
		}
	}

	free(signals);
	return MUNIT_OK;
}

static MunitResult benchmark_write_uint8_masked(const MunitParameter params[], void* user_data_or_fixture) {
	SignalPool *pool = (SignalPool *) user_data_or_fixture;

	Signal *signals = (Signal *) malloc(sizeof(Signal) * 1024);
	for (int i = 0; i < 1024; ++i) {
		signals[i] = signal_create(pool, 8);
	}

	for (int j = 0; j < 102400; ++j) {
		for (int i = 0; i < 1024; ++i) {
			signal_write_uint8_masked(pool, signals[i], j & 255, (j >> 2) & 255, 0);
		}
	}

	free(signals);
	return MUNIT_OK;
}

MunitTest signal_tests[] = {
    { "/create", test_create, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read_bool", test_read_bool, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read_uint8", test_read_uint8, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/read_uint16", test_read_uint8, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/write_bool", test_write_bool, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/write_uint8", test_write_uint8, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/write_uint16", test_write_uint16, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
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
    { "/clear_writer", test_clear_writer, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
	{ "/changed", test_changed, signal_setup, signal_teardown, MUNIT_TEST_OPTION_NONE, NULL },
    { "/names", test_names, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/fetch_by_name", test_fetch_by_name, signal_setup, signal_teardown,  MUNIT_TEST_OPTION_NONE, NULL },
//	{ "/benchmark_read_uint8", benchmark_read_uint8, signal_setup, signal_teardown, MUNIT_TEST_OPTION_NONE, NULL},
//	{ "/benchmark_write_uint8", benchmark_write_uint8, signal_setup, signal_teardown, MUNIT_TEST_OPTION_NONE, NULL},
//	{ "/benchmark_write_uint8_masked", benchmark_write_uint8_masked, signal_setup, signal_teardown, MUNIT_TEST_OPTION_NONE, NULL},
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
