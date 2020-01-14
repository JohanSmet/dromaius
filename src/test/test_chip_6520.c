// test/test_chip_6520.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_6520.h"

#include "stb/stb_ds.h"

#define SIGNAL_POOL			pia->signal_pool
#define SIGNAL_COLLECTION	pia->signals

static void *chip_6520_setup(const MunitParameter params[], void *user_data) {
	return chip_6520_create(signal_pool_create(), (Chip6520Signals) {0});
}

static void chip_6520_teardown(void *fixture) {
	chip_6520_destroy((Chip6520 *) fixture);
}

static MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// force registers to junk values
	pia->reg_ddra = 0xde;
	pia->reg_cra.reg = 0xad;
	pia->reg_ora = 0xbe;

	pia->reg_ddrb = 0xef;
	pia->reg_crb.reg = 0xba;
	pia->reg_orb = 0xdd;

	// run pia with reset asserted
	SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
	signal_pool_cycle(pia->signal_pool);
	chip_6520_process(pia);

	// registers should have been reset
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);

	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

MunitTest chip_6520_tests[] = {
	{ "/reset", test_reset, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
