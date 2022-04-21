// test/test_chip_oscillator.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_mc3446a.h"

#include "crt.h"
#include "simulator.h"
#include "fixture_chip.h"

#define SIGNAL_PREFIX		CHIP_MC3446A_
#define SIGNAL_OWNER		fixture->chip_tester
static void chip_tester_destroy(Chip *chip) {
	assert(chip);
	dms_free(chip);
}

static void chip_tester_process(Chip *chip) {
	assert(chip);
}

static void *chip_mc3446a_setup(const MunitParameter params[], void *user_data) {

	TestFixture *fixture = fixture_chip_create();

	// create chip to be tested
	fixture->chip_under_test = (Chip *) chip_mc3446a_create(fixture->simulator, (ChipMC3446ASignals) {0});
	simulator_register_chip(fixture->simulator, fixture->chip_under_test, "cut");

	// create tester chip
	fixture_chip_create_tester(fixture);

	return fixture;
}

static void chip_mc3446a_teardown(void *munit_fixture) {
	fixture_chip_destroy(munit_fixture);
}

static MunitResult test_input_to_bus(const MunitParameter params[], void *user_data_or_fixture) {

	TestFixture *fixture = (TestFixture *) user_data_or_fixture;
	ChipMC3446A *chip = (ChipMC3446A *) fixture->chip_under_test;
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->destroy);

	// initialize all outgoing (input -> bus) signals to high
	SIGNAL_WRITE(ABCE_B, ACTLO_DEASSERT);
	SIGNAL_WRITE(DE_B, ACTLO_DEASSERT);
	SIGNAL_WRITE(AI, true);
	SIGNAL_WRITE(BI, true);
	SIGNAL_WRITE(CI, true);
	SIGNAL_WRITE(DI, true);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(AB));
	munit_assert_true(SIGNAL_READ_NEXT(BB));
	munit_assert_true(SIGNAL_READ_NEXT(CB));
	munit_assert_true(SIGNAL_READ_NEXT(DB));

	// going low on xI signals shouldn't do anything if enable_b signals are high
	SIGNAL_WRITE(AI, false);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(AB));
	munit_assert_true(SIGNAL_READ_NEXT(BB));
	munit_assert_true(SIGNAL_READ_NEXT(CB));
	munit_assert_true(SIGNAL_READ_NEXT(DB));

	SIGNAL_WRITE(BI, false);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(AB));
	munit_assert_true(SIGNAL_READ_NEXT(BB));
	munit_assert_true(SIGNAL_READ_NEXT(CB));
	munit_assert_true(SIGNAL_READ_NEXT(DB));

	SIGNAL_WRITE(CI, false);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(AB));
	munit_assert_true(SIGNAL_READ_NEXT(BB));
	munit_assert_true(SIGNAL_READ_NEXT(CB));
	munit_assert_true(SIGNAL_READ_NEXT(DB));

	SIGNAL_WRITE(DI, false);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(AB));
	munit_assert_true(SIGNAL_READ_NEXT(BB));
	munit_assert_true(SIGNAL_READ_NEXT(CB));
	munit_assert_true(SIGNAL_READ_NEXT(DB));

	// enable transmission of first three lines
	SIGNAL_WRITE(ABCE_B, ACTLO_ASSERT);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_false(SIGNAL_READ_NEXT(AB));
	munit_assert_false(SIGNAL_READ_NEXT(BB));
	munit_assert_false(SIGNAL_READ_NEXT(CB));
	munit_assert_true(SIGNAL_READ_NEXT(DB));

	SIGNAL_WRITE(BI, true);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_false(SIGNAL_READ_NEXT(AB));
	munit_assert_true(SIGNAL_READ_NEXT(BB));
	munit_assert_false(SIGNAL_READ_NEXT(CB));
	munit_assert_true(SIGNAL_READ_NEXT(DB));

	// enable transmission of the fourth line
	SIGNAL_WRITE(DE_B, ACTLO_ASSERT);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_false(SIGNAL_READ_NEXT(AB));
	munit_assert_true(SIGNAL_READ_NEXT(BB));
	munit_assert_false(SIGNAL_READ_NEXT(CB));
	munit_assert_false(SIGNAL_READ_NEXT(DB));

	SIGNAL_WRITE(DI, true);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_false(SIGNAL_READ_NEXT(AB));
	munit_assert_true(SIGNAL_READ_NEXT(BB));
	munit_assert_false(SIGNAL_READ_NEXT(CB));
	munit_assert_true(SIGNAL_READ_NEXT(DB));

	return MUNIT_OK;
}

static MunitResult test_bus_to_output(const MunitParameter params[], void *user_data_or_fixture) {

	TestFixture *fixture = (TestFixture *) user_data_or_fixture;
	ChipMC3446A *chip = (ChipMC3446A *) fixture->chip_under_test;
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->destroy);

	// make sure transmission (input -> bus) is disabled
	SIGNAL_WRITE(ABCE_B, ACTLO_DEASSERT);
	SIGNAL_WRITE(DE_B, ACTLO_DEASSERT);
	SIGNAL_NO_WRITE(AB);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(AO));
	munit_assert_true(SIGNAL_READ_NEXT(BO));
	munit_assert_true(SIGNAL_READ_NEXT(CO));
	munit_assert_true(SIGNAL_READ_NEXT(DO));

	// write the bus-A
	SIGNAL_WRITE(AB, false);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_false(SIGNAL_READ_NEXT(AO));
	munit_assert_true(SIGNAL_READ_NEXT(BO));
	munit_assert_true(SIGNAL_READ_NEXT(CO));
	munit_assert_true(SIGNAL_READ_NEXT(DO));

	// write the bus-B
	SIGNAL_WRITE(AB, true);
	SIGNAL_WRITE(BB, false);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(AO));
	munit_assert_false(SIGNAL_READ_NEXT(BO));
	munit_assert_true(SIGNAL_READ_NEXT(CO));
	munit_assert_true(SIGNAL_READ_NEXT(DO));

	// write the bus-C
	SIGNAL_WRITE(BB, true);
	SIGNAL_WRITE(CB, false);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(AO));
	munit_assert_true(SIGNAL_READ_NEXT(BO));
	munit_assert_false(SIGNAL_READ_NEXT(CO));
	munit_assert_true(SIGNAL_READ_NEXT(DO));

	// write the bus-D
	SIGNAL_WRITE(CB, true);
	SIGNAL_WRITE(DB, false);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(AO));
	munit_assert_true(SIGNAL_READ_NEXT(BO));
	munit_assert_true(SIGNAL_READ_NEXT(CO));
	munit_assert_false(SIGNAL_READ_NEXT(DO));

	return MUNIT_OK;
}

MunitTest chip_mc3446a_tests[] = {
	{ "/input_to_bus", test_input_to_bus, chip_mc3446a_setup, chip_mc3446a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/bus_to_output", test_bus_to_output, chip_mc3446a_setup, chip_mc3446a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
