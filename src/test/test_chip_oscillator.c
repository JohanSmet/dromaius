// test/test_chip_oscillator.c - Johan Smet - BSD-3-Clause (see LICENSE)

#define SIGNAL_ARRAY_STYLE
#include "munit/munit.h"
#include "chip_oscillator.h"
#include "simulator.h"

#define SIGNAL_PREFIX		CHIP_OSCILLATOR_
#define SIGNAL_OWNER		tmr

static MunitResult test_frequency(const MunitParameter params[], void *user_data_or_fixture) {

	Simulator *sim_1 = simulator_create(1);

	Oscillator *tmr_1mhz = oscillator_create(1000000, sim_1, (OscillatorSignals) {0});
	munit_assert_int64(tmr_1mhz->half_period_ticks, ==, 1000000 / 2);
	tmr_1mhz->destroy(tmr_1mhz);

	Oscillator *tmr_16mhz = oscillator_create(16000000, sim_1, (OscillatorSignals) {0});
	munit_assert_int64(tmr_16mhz->half_period_ticks, ==, 62500 / 2);
	tmr_16mhz->destroy(tmr_16mhz);

	Oscillator *tmr_400mhz = oscillator_create(400000000, sim_1, (OscillatorSignals) {0});
	munit_assert_int64(tmr_400mhz->half_period_ticks, ==, 2500 / 2);
	tmr_400mhz->destroy(tmr_400mhz);

	Simulator *sim_100 = simulator_create(100);

	Oscillator *tmr_1mhz_b = oscillator_create(1000000, sim_100, (OscillatorSignals) {0});
	munit_assert_int64(tmr_1mhz_b->half_period_ticks, ==, 10000 / 2);
	tmr_1mhz_b->destroy(tmr_1mhz_b);

	simulator_destroy(sim_1);
	simulator_destroy(sim_100);

	return MUNIT_OK;
}

static MunitResult test_clock(const MunitParameter params[], void *user_data_or_fixture) {

	Simulator *sim = simulator_create(1000);

	Oscillator *tmr = oscillator_create(1000000, sim, (OscillatorSignals) {0});

	signal_pool_cycle(tmr->signal_pool, sim->current_tick);
	tmr->process(tmr);
	munit_assert_false(SIGNAL_READ_NEXT(CLK_OUT));

	sim->current_tick += 250;
	signal_pool_cycle(tmr->signal_pool, sim->current_tick);
	tmr->process(tmr);
	munit_assert_false(SIGNAL_READ_NEXT(CLK_OUT));

	sim->current_tick += 250;
	signal_pool_cycle(tmr->signal_pool, sim->current_tick);
	tmr->process(tmr);
	munit_assert_true(SIGNAL_READ_NEXT(CLK_OUT));

	sim->current_tick += 250;
	signal_pool_cycle(tmr->signal_pool, sim->current_tick);
	tmr->process(tmr);
	munit_assert_true(SIGNAL_READ_NEXT(CLK_OUT));

	sim->current_tick += 250;
	signal_pool_cycle(tmr->signal_pool, sim->current_tick);
	tmr->process(tmr);
	munit_assert_false(SIGNAL_READ_NEXT(CLK_OUT));

	simulator_destroy(sim);
	tmr->destroy(tmr);

	return MUNIT_OK;
}


MunitTest chip_oscillator_tests[] = {
	{ "/frequency", test_frequency, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/clock", test_clock, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
