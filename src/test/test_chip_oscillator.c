// test/test_chip_oscillator.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_oscillator.h"
#include "simulator.h"

#define SIGNAL_POOL			tmr->signal_pool
#define SIGNAL_COLLECTION	tmr->signals

static MunitResult test_frequency(const MunitParameter params[], void *user_data_or_fixture) {

	Simulator *sim_1 = simulator_create(1);

	Oscillator *tmr_1mhz = oscillator_create(1000000, sim_1, (OscillatorSignals) {0});
	munit_assert_int64(tmr_1mhz->half_period_ticks, ==, 1000000 / 2);
	oscillator_destroy(tmr_1mhz);

	Oscillator *tmr_16mhz = oscillator_create(16000000, sim_1, (OscillatorSignals) {0});
	munit_assert_int64(tmr_16mhz->half_period_ticks, ==, 62500 / 2);
	oscillator_destroy(tmr_16mhz);

	Oscillator *tmr_400mhz = oscillator_create(400000000, sim_1, (OscillatorSignals) {0});
	munit_assert_int64(tmr_400mhz->half_period_ticks, ==, 2500 / 2);
	oscillator_destroy(tmr_400mhz);

	Simulator *sim_100 = simulator_create(100);

	Oscillator *tmr_1mhz_b = oscillator_create(1000000, sim_100, (OscillatorSignals) {0});
	munit_assert_int64(tmr_1mhz_b->half_period_ticks, ==, 10000 / 2);
	oscillator_destroy(tmr_1mhz_b);

	simulator_destroy(sim_1);
	simulator_destroy(sim_100);

	return MUNIT_OK;
}

static MunitResult test_clock(const MunitParameter params[], void *user_data_or_fixture) {

	Simulator *sim = simulator_create(1000);

	Oscillator *tmr = oscillator_create(1000000, sim, (OscillatorSignals) {0});

	signal_pool_cycle(tmr->signal_pool, sim->current_tick);
	tmr->process(tmr);
	munit_assert_false(SIGNAL_NEXT_BOOL(clk_out));

	sim->current_tick += 250;
	signal_pool_cycle(tmr->signal_pool, sim->current_tick);
	tmr->process(tmr);
	munit_assert_false(SIGNAL_NEXT_BOOL(clk_out));

	sim->current_tick += 250;
	signal_pool_cycle(tmr->signal_pool, sim->current_tick);
	tmr->process(tmr);
	munit_assert_true(SIGNAL_NEXT_BOOL(clk_out));

	sim->current_tick += 250;
	signal_pool_cycle(tmr->signal_pool, sim->current_tick);
	tmr->process(tmr);
	munit_assert_true(SIGNAL_NEXT_BOOL(clk_out));

	sim->current_tick += 250;
	signal_pool_cycle(tmr->signal_pool, sim->current_tick);
	tmr->process(tmr);
	munit_assert_false(SIGNAL_NEXT_BOOL(clk_out));

	simulator_destroy(sim);
	oscillator_destroy(tmr);

	return MUNIT_OK;
}


MunitTest chip_oscillator_tests[] = {
	{ "/frequency", test_frequency, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/clock", test_clock, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
