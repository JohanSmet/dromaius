// test/test_clock.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"

#include "clock.h"

static void *clock_setup(const MunitParameter params[], void *user_data) {
	return clock_create(10, signal_pool_create(), (ClockSignals) {0});
}

static void clock_teardown(void *fixture) {
	clock_destroy((Clock *) fixture);
}

MunitResult test_cycle_count(const MunitParameter params[], void* user_data_or_fixture) {
	
	Clock *clock = (Clock *) user_data_or_fixture;

	munit_assert_int64(clock->cycle_count, ==, 0);
	clock_process(clock);
	signal_pool_cycle(clock->signal_pool);
	munit_assert_int64(clock->cycle_count, ==, 1);
	clock_process(clock);
	signal_pool_cycle(clock->signal_pool);
	munit_assert_int64(clock->cycle_count, ==, 1);
	clock_process(clock);
	signal_pool_cycle(clock->signal_pool);
	munit_assert_int64(clock->cycle_count, ==, 2);
	clock_process(clock);
	signal_pool_cycle(clock->signal_pool);
	munit_assert_int64(clock->cycle_count, ==, 2);

	return MUNIT_OK;
}

MunitResult test_frequency(const MunitParameter params[], void* user_data_or_fixture) {
	
	Clock *clock = (Clock *) user_data_or_fixture;

	clock_set_frequency(clock, 500);			// 500 Hz => clock changes every millisecond
	munit_assert_uint32(clock->conf_frequency, ==, 500);
	munit_assert_uint64(clock->conf_half_period_ns, ==, 1000000);
	
	clock_set_frequency(clock, 1000000);		// 1 MHz
	munit_assert_uint32(clock->conf_frequency, ==, 1000000);
	munit_assert_uint64(clock->conf_half_period_ns, ==, 500);

	return MUNIT_OK;
}

MunitTest clock_tests[] = {
    { "/cycle_count", test_cycle_count, clock_setup, clock_teardown, MUNIT_TEST_OPTION_NONE, NULL },
    { "/frequency", test_frequency, clock_setup, clock_teardown, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
