// test/test_chip_poweronreset.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_poweronreset.h"
#include "simulator.h"

#define SIGNAL_POOL			por->signal_pool
#define SIGNAL_COLLECTION	por->signals
#define SIGNAL_CHIP_ID		por->id

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

static MunitResult test_trigger(const MunitParameter params[], void *user_data_or_fixture) {

	Simulator *sim = simulator_create(1000);
	PowerOnReset *por = poweronreset_create(250000, sim, (PowerOnResetSignals) {0});

	SIGNAL_SET_BOOL(trigger_b, true);
	SIGNAL_SET_BOOL(reset_b, true);
	signal_pool_cycle(por->signal_pool, sim->current_tick);

	sim->current_tick += 100;
	por->process(por);
	munit_assert_false(SIGNAL_NEXT_BOOL(reset_b));

	sim->current_tick += 200;
	por->process(por);
	munit_assert_true(SIGNAL_NEXT_BOOL(reset_b));

	sim->current_tick += 50;
	SIGNAL_SET_BOOL(trigger_b, false);
	signal_pool_cycle(por->signal_pool, sim->current_tick);
	por->process(por);
	munit_assert_false(SIGNAL_NEXT_BOOL(reset_b));

	sim->current_tick += 50;
	SIGNAL_SET_BOOL(trigger_b, true);
	signal_pool_cycle(por->signal_pool, sim->current_tick);
	por->process(por);
	munit_assert_false(SIGNAL_NEXT_BOOL(reset_b));

	sim->current_tick += 100;
	por->process(por);
	munit_assert_false(SIGNAL_NEXT_BOOL(reset_b));

	sim->current_tick += 100;
	por->process(por);
	munit_assert_true(SIGNAL_NEXT_BOOL(reset_b));

	sim->current_tick += 50;
	por->process(por);
	munit_assert_true(SIGNAL_NEXT_BOOL(reset_b));

	return MUNIT_OK;
}

MunitTest chip_poweronreset_tests[] = {
	{ "/reset", test_reset, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/trigger", test_trigger, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
