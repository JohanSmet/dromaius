// test/test_chip_poweronreset.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_poweronreset.h"

#define SIGNAL_POOL			por->signal_pool
#define SIGNAL_COLLECTION	por->signals

static MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {

	SignalPool *pool = signal_pool_create(1);
	pool->tick_duration_ps = 1000;

	Signal sig_reset_b = signal_create(pool, 1);
	signal_default_bool(pool, sig_reset_b, ACTLO_ASSERT);

	PowerOnReset *por = poweronreset_create(250000, pool, (PowerOnResetSignals) {sig_reset_b});

	munit_assert_false(SIGNAL_NEXT_BOOL(reset_b));

	por->signal_pool->current_tick += 100;
	por->process(por);
	munit_assert_false(SIGNAL_NEXT_BOOL(reset_b));

	por->signal_pool->current_tick += 100;
	por->process(por);
	munit_assert_false(SIGNAL_NEXT_BOOL(reset_b));

	por->signal_pool->current_tick += 100;
	por->process(por);
	munit_assert_true(SIGNAL_NEXT_BOOL(reset_b));

	por->signal_pool->current_tick += 100;
	por->process(por);
	munit_assert_true(SIGNAL_NEXT_BOOL(reset_b));

	return MUNIT_OK;
}


MunitTest chip_poweronreset_tests[] = {
	{ "/reset", test_reset, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
