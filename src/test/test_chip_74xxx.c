// test/test_chip_74xxx.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_74xxx.h"

#define SIGNAL_POOL			chip->signal_pool
#define SIGNAL_COLLECTION	chip->signals

static MunitResult test_7400_nand(const MunitParameter params[], void *user_data_or_fixture) {

	bool truth_table[4][3] = {
		false, false, true,
		false, true,  true,
		true,  false, true,
		true,  true,  false
	};

	Chip7400Nand *chip = chip_7400_nand_create(signal_pool_create(), (Chip7400NandSignals) {0});

	for (int i = 0; i < 4; ++i) {
		SIGNAL_SET_BOOL(a1, truth_table[(i+0)%4][0]);
		SIGNAL_SET_BOOL(b1, truth_table[(i+0)%4][1]);
		SIGNAL_SET_BOOL(a2, truth_table[(i+1)%4][0]);
		SIGNAL_SET_BOOL(b2, truth_table[(i+1)%4][1]);
		SIGNAL_SET_BOOL(a3, truth_table[(i+2)%4][0]);
		SIGNAL_SET_BOOL(b3, truth_table[(i+2)%4][1]);
		SIGNAL_SET_BOOL(a4, truth_table[(i+3)%4][0]);
		SIGNAL_SET_BOOL(b4, truth_table[(i+3)%4][1]);
		chip_7400_nand_process(chip);

		munit_assert(SIGNAL_NEXT_BOOL(y1) == truth_table[(i+0)%4][2]);
		munit_assert(SIGNAL_NEXT_BOOL(y2) == truth_table[(i+1)%4][2]);
		munit_assert(SIGNAL_NEXT_BOOL(y3) == truth_table[(i+2)%4][2]);
		munit_assert(SIGNAL_NEXT_BOOL(y4) == truth_table[(i+3)%4][2]);

		signal_pool_cycle(chip->signal_pool);
	}

	chip_7400_nand_destroy(chip);
	return MUNIT_OK;
}

MunitTest chip_74xxx_tests[] = {
	{ "/7400_nand", test_7400_nand, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
