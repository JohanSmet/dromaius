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

	Chip7400Nand *chip = chip_7400_nand_create(signal_pool_create(1), (Chip7400NandSignals) {0});

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

static MunitResult test_74145_bcd_decoder(const MunitParameter params[], void *user_data_or_fixture) {
	Chip74145BcdDecoder *chip = chip_74145_bcd_decoder_create(signal_pool_create(1), (Chip74145Signals) {0});

	for (int input = 0; input < 16; ++input) {
		SIGNAL_SET_BOOL(a, BIT_IS_SET(input, 0));
		SIGNAL_SET_BOOL(b, BIT_IS_SET(input, 1));
		SIGNAL_SET_BOOL(c, BIT_IS_SET(input, 2));
		SIGNAL_SET_BOOL(d, BIT_IS_SET(input, 3));

		chip_74145_bcd_decoder_process(chip);

		munit_assert(SIGNAL_NEXT_BOOL(y0_b) == ((input == 0) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_NEXT_BOOL(y1_b) == ((input == 1) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_NEXT_BOOL(y2_b) == ((input == 2) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_NEXT_BOOL(y3_b) == ((input == 3) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_NEXT_BOOL(y4_b) == ((input == 4) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_NEXT_BOOL(y5_b) == ((input == 5) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_NEXT_BOOL(y6_b) == ((input == 6) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_NEXT_BOOL(y7_b) == ((input == 7) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_NEXT_BOOL(y8_b) == ((input == 8) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_NEXT_BOOL(y9_b) == ((input == 9) ? ACTLO_ASSERT : ACTLO_DEASSERT));

		signal_pool_cycle(chip->signal_pool);
	}

	chip_74145_bcd_decoder_destroy(chip);
	return MUNIT_OK;
}

static MunitResult test_74154_decoder(const MunitParameter params[], void *user_data_or_fixture) {
// TODO: ATM it's not possible to test high impedance output state correctly

	Chip74154Decoder *chip = chip_74154_decoder_create(signal_pool_create(1), (Chip74154Signals) {0});

	bool truth_table[4][2] = {
		false, false,
		false, true,
		true,  false,
		true,  true
	};

	for (int i = 0; i < 4; ++i) {
		for (int input = 0; input < 16; ++input) {
			SIGNAL_SET_BOOL(g1_b, truth_table[i][0]);
			SIGNAL_SET_BOOL(g2_b, truth_table[i][1]);
			SIGNAL_SET_BOOL(a, BIT_IS_SET(input, 0));
			SIGNAL_SET_BOOL(b, BIT_IS_SET(input, 1));
			SIGNAL_SET_BOOL(c, BIT_IS_SET(input, 2));
			SIGNAL_SET_BOOL(d, BIT_IS_SET(input, 3));

			chip_74154_decoder_process(chip);

			bool strobed = !truth_table[i][0] && !truth_table[i][1];
			munit_assert(SIGNAL_NEXT_BOOL(y0_b) == ((!strobed || input == 0) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y1_b) == ((!strobed || input == 1) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y2_b) == ((!strobed || input == 2) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y3_b) == ((!strobed || input == 3) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y4_b) == ((!strobed || input == 4) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y5_b) == ((!strobed || input == 5) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y6_b) == ((!strobed || input == 6) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y7_b) == ((!strobed || input == 7) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y8_b) == ((!strobed || input == 8) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y9_b) == ((!strobed || input == 9) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y10_b) == ((!strobed || input == 10) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y11_b) == ((!strobed || input == 11) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y12_b) == ((!strobed || input == 12) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y13_b) == ((!strobed || input == 13) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y14_b) == ((!strobed || input == 14) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y15_b) == ((!strobed || input == 15) ? ACTLO_ASSERT : ACTLO_DEASSERT));

			signal_pool_cycle(chip->signal_pool);
		}
	}

	chip_74154_decoder_destroy(chip);
	return MUNIT_OK;
}

static MunitResult test_74164_shift_register(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74164ShiftRegister *chip = chip_74164_shift_register_create(signal_pool_create(1), (Chip74164Signals) {0});

	chip->state = 0xaf;				// 'random' startup state

	bool truth_table[][10] = {		// replicates timing diagram from datasheet
		//  A      B     Qa     Qb     Qc     Qd     Qe     Qf     Qg     Qh
		{true,  false, false, false, false, false, false, false, false, false},		// 01
		{false, true,  false, false, false, false, false, false, false, false},		// 02
		{false, true,  false, false, false, false, false, false, false, false},		// 03
		{true,  true,  true,  false, false, false, false, false, false, false},		// 04
		{true,  true,  true,  true,  false, false, false, false, false, false},		// 05
		{false, true,  false, true,  true,  false, false, false, false, false},		// 06
		{true,  true,  true,  false, true,  true,  false, false, false, false},		// 07
		{false, true,  false, true,  false, true,  true,  false, false, false},		// 08
		{false, true,  false, false, true,  false, true,  true,  false, false},		// 09
		{false, true,  false, false, false, true,  false, true,  true,  false},		// 10
		{false, true,  false, false, false, false, true,  false, true,  true}		// 11
	};

	// reset the shift register
	SIGNAL_SET_BOOL(clear_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clk, false);
	signal_pool_cycle(chip->signal_pool);
	chip_74164_shift_register_process(chip);
	munit_assert_uint8(chip->state, ==, 0);
	munit_assert_false(SIGNAL_NEXT_BOOL(qa));
	munit_assert_false(SIGNAL_NEXT_BOOL(qb));
	munit_assert_false(SIGNAL_NEXT_BOOL(qc));
	munit_assert_false(SIGNAL_NEXT_BOOL(qd));
	munit_assert_false(SIGNAL_NEXT_BOOL(qe));
	munit_assert_false(SIGNAL_NEXT_BOOL(qf));
	munit_assert_false(SIGNAL_NEXT_BOOL(qg));
	munit_assert_false(SIGNAL_NEXT_BOOL(qh));

	// run truth table
	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "truth_table[%d] - a = %d, b = %d", line, truth_table[line][0], truth_table[line][1]);

		// positive edge
		SIGNAL_SET_BOOL(clear_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk, true);
		SIGNAL_SET_BOOL(a, truth_table[line][0]);
		SIGNAL_SET_BOOL(b, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool);
		chip_74164_shift_register_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(qa) == truth_table[line][2]);
		munit_assert(SIGNAL_NEXT_BOOL(qb) == truth_table[line][3]);
		munit_assert(SIGNAL_NEXT_BOOL(qc) == truth_table[line][4]);
		munit_assert(SIGNAL_NEXT_BOOL(qd) == truth_table[line][5]);
		munit_assert(SIGNAL_NEXT_BOOL(qe) == truth_table[line][6]);
		munit_assert(SIGNAL_NEXT_BOOL(qf) == truth_table[line][7]);
		munit_assert(SIGNAL_NEXT_BOOL(qg) == truth_table[line][8]);
		munit_assert(SIGNAL_NEXT_BOOL(qh) == truth_table[line][9]);

		// negative edge
		SIGNAL_SET_BOOL(clear_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk, false);
		SIGNAL_SET_BOOL(a, truth_table[line][0]);
		SIGNAL_SET_BOOL(b, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool);
		chip_74164_shift_register_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(qa) == truth_table[line][2]);
		munit_assert(SIGNAL_NEXT_BOOL(qb) == truth_table[line][3]);
		munit_assert(SIGNAL_NEXT_BOOL(qc) == truth_table[line][4]);
		munit_assert(SIGNAL_NEXT_BOOL(qd) == truth_table[line][5]);
		munit_assert(SIGNAL_NEXT_BOOL(qe) == truth_table[line][6]);
		munit_assert(SIGNAL_NEXT_BOOL(qf) == truth_table[line][7]);
		munit_assert(SIGNAL_NEXT_BOOL(qg) == truth_table[line][8]);
		munit_assert(SIGNAL_NEXT_BOOL(qh) == truth_table[line][9]);
	}

	// reset the shift register again
	SIGNAL_SET_BOOL(clear_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clk, false);
	signal_pool_cycle(chip->signal_pool);
	chip_74164_shift_register_process(chip);
	munit_assert_uint8(chip->state, ==, 0);
	munit_assert_false(SIGNAL_NEXT_BOOL(qa));
	munit_assert_false(SIGNAL_NEXT_BOOL(qb));
	munit_assert_false(SIGNAL_NEXT_BOOL(qc));
	munit_assert_false(SIGNAL_NEXT_BOOL(qd));
	munit_assert_false(SIGNAL_NEXT_BOOL(qe));
	munit_assert_false(SIGNAL_NEXT_BOOL(qf));
	munit_assert_false(SIGNAL_NEXT_BOOL(qg));
	munit_assert_false(SIGNAL_NEXT_BOOL(qh));

	chip_74164_shift_register_destroy(chip);

	return MUNIT_OK;
}

static MunitResult test_74244_octal_buffer(const MunitParameter params[], void *user_data_or_fixture) {
// TODO: ATM it's not possible to test high impedance output state correctly

	bool truth_table[4][2] = {
		false, false,
		false, true,
		true,  false,
		true,  true
	};

	Chip74244OctalBuffer *chip = chip_74244_octal_buffer_create(signal_pool_create(1), (Chip74244Signals) {0});

	for (int i = 0; i < 4; ++i) {

		for (int input = 0; input < 256; ++input) {
			SIGNAL_SET_BOOL(g1_b, truth_table[i][0]);
			SIGNAL_SET_BOOL(g2_b, truth_table[i][1]);

			SIGNAL_SET_BOOL(a11, BIT_IS_SET(input, 0));
			SIGNAL_SET_BOOL(a12, BIT_IS_SET(input, 1));
			SIGNAL_SET_BOOL(a13, BIT_IS_SET(input, 2));
			SIGNAL_SET_BOOL(a14, BIT_IS_SET(input, 3));
			SIGNAL_SET_BOOL(a21, BIT_IS_SET(input, 4));
			SIGNAL_SET_BOOL(a22, BIT_IS_SET(input, 5));
			SIGNAL_SET_BOOL(a23, BIT_IS_SET(input, 6));
			SIGNAL_SET_BOOL(a24, BIT_IS_SET(input, 7));

			chip_74244_octal_buffer_process(chip);

			munit_assert(SIGNAL_NEXT_BOOL(y11) == (truth_table[i][0] ? false : BIT_IS_SET(input, 0)));
			munit_assert(SIGNAL_NEXT_BOOL(y12) == (truth_table[i][0] ? false : BIT_IS_SET(input, 1)));
			munit_assert(SIGNAL_NEXT_BOOL(y13) == (truth_table[i][0] ? false : BIT_IS_SET(input, 2)));
			munit_assert(SIGNAL_NEXT_BOOL(y14) == (truth_table[i][0] ? false : BIT_IS_SET(input, 3)));
			munit_assert(SIGNAL_NEXT_BOOL(y21) == (truth_table[i][1] ? false : BIT_IS_SET(input, 4)));
			munit_assert(SIGNAL_NEXT_BOOL(y22) == (truth_table[i][1] ? false : BIT_IS_SET(input, 5)));
			munit_assert(SIGNAL_NEXT_BOOL(y23) == (truth_table[i][1] ? false : BIT_IS_SET(input, 6)));
			munit_assert(SIGNAL_NEXT_BOOL(y24) == (truth_table[i][1] ? false : BIT_IS_SET(input, 7)));

			signal_pool_cycle(chip->signal_pool);
		}
	}

	chip_74244_octal_buffer_destroy(chip);

	return MUNIT_OK;
}

MunitTest chip_74xxx_tests[] = {
	{ "/7400_nand", test_7400_nand, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74145_bcd_decoder", test_74145_bcd_decoder, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74154_decoder", test_74154_decoder, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74164_shift_register", test_74164_shift_register, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74244_octal_buffer", test_74244_octal_buffer, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
