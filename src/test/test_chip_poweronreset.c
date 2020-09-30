// test/test_chip_poweronreset.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_poweronreset.h"
#include "simulator.h"

#define SIGNAL_POOL			por->signal_pool
#define SIGNAL_COLLECTION	por->signals

static MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {

	Simulator *sim = simulator_create(1000);

	Signal sig_reset_b = signal_create(sim->signal_pool, 1);
	signal_default_bool(sim->signal_pool, sig_reset_b, ACTLO_ASSERT);

	PowerOnReset *por = poweronreset_create(250000, sim, (PowerOnResetSignals) {sig_reset_b});

	munit_assert_false(SIGNAL_NEXT_BOOL(reset_b));

	sim->current_tick += 100;
	por->process(por);
	munit_assert_false(SIGNAL_NEXT_BOOL(reset_b));

	sim->current_tick += 100;
	por->process(por);
	munit_assert_false(SIGNAL_NEXT_BOOL(reset_b));

	sim->current_tick += 100;
	por->process(por);
	munit_assert_true(SIGNAL_NEXT_BOOL(reset_b));

	sim->current_tick += 100;
	por->process(por);
	munit_assert_true(SIGNAL_NEXT_BOOL(reset_b));

	return MUNIT_OK;
}


MunitTest chip_poweronreset_tests[] = {
	{ "/reset", test_reset, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
