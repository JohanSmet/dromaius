// test/test_chip_74xxx.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_74xxx.h"
#include "simulator.h"

#define SIGNAL_OWNER		chip
#define SIGNAL_PREFIX		CHIP_7400_

static MunitResult test_7400_nand(const MunitParameter params[], void *user_data_or_fixture) {

	bool truth_table[4][3] = {
		false, false, true,
		false, true,  true,
		true,  false, true,
		true,  true,  false
	};

	Chip7400Nand *chip = chip_7400_nand_create(simulator_create(NS_TO_PS(100)), (Chip7400Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	for (int i = 0; i < 4; ++i) {
		SIGNAL_WRITE(A1, truth_table[(i+0)%4][0]);
		SIGNAL_WRITE(B1, truth_table[(i+0)%4][1]);
		SIGNAL_WRITE(A2, truth_table[(i+1)%4][0]);
		SIGNAL_WRITE(B2, truth_table[(i+1)%4][1]);
		SIGNAL_WRITE(A3, truth_table[(i+2)%4][0]);
		SIGNAL_WRITE(B3, truth_table[(i+2)%4][1]);
		SIGNAL_WRITE(A4, truth_table[(i+3)%4][0]);
		SIGNAL_WRITE(B4, truth_table[(i+3)%4][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);

		munit_assert(SIGNAL_READ_NEXT(Y1) == truth_table[(i+0)%4][2]);
		munit_assert(SIGNAL_READ_NEXT(Y2) == truth_table[(i+1)%4][2]);
		munit_assert(SIGNAL_READ_NEXT(Y3) == truth_table[(i+2)%4][2]);
		munit_assert(SIGNAL_READ_NEXT(Y4) == truth_table[(i+3)%4][2]);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_7474_

static MunitResult test_7474_d_flipflop(const MunitParameter params[], void *user_data_or_fixture) {
	Chip7474DFlipFlop *chip = chip_7474_d_flipflop_create(simulator_create(NS_TO_PS(100)), (Chip7474Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	// set both flip-flops
	SIGNAL_WRITE(PR1_B, ACTLO_ASSERT);
	SIGNAL_WRITE(PR2_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLR1_B, ACTLO_DEASSERT);
	SIGNAL_WRITE(CLR2_B, ACTLO_DEASSERT);
	SIGNAL_WRITE(CLK1, false);
	SIGNAL_WRITE(CLK2, false);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(Q1));
	munit_assert_false(SIGNAL_READ_NEXT(Q1_B));
	munit_assert_true(SIGNAL_READ_NEXT(Q2));
	munit_assert_false(SIGNAL_READ_NEXT(Q2_B));

	// check nonstable state when both clear and preset are asserted
	SIGNAL_WRITE(PR1_B, ACTLO_ASSERT);
	SIGNAL_WRITE(PR2_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLR1_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLR2_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLK1, false);
	SIGNAL_WRITE(CLK2, false);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(Q1));
	munit_assert_true(SIGNAL_READ_NEXT(Q1_B));
	munit_assert_true(SIGNAL_READ_NEXT(Q2));
	munit_assert_true(SIGNAL_READ_NEXT(Q2_B));

	// clear both flip-flops
	SIGNAL_WRITE(PR1_B, ACTLO_DEASSERT);
	SIGNAL_WRITE(PR2_B, ACTLO_DEASSERT);
	SIGNAL_WRITE(CLR1_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLR2_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLK1, false);
	SIGNAL_WRITE(CLK2, false);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_false(SIGNAL_READ_NEXT(Q1));
	munit_assert_true(SIGNAL_READ_NEXT(Q1_B));
	munit_assert_false(SIGNAL_READ_NEXT(Q2));
	munit_assert_true(SIGNAL_READ_NEXT(Q2_B));

	bool truth_table[][3] = {
		// d      q     q_b
		{true,  true,  false},
		{false, false, true}
	};

	// test flip-flop 1
	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "1: truth_table[%d] - d = %d", line, truth_table[line][0]);

		// positive edge
		SIGNAL_WRITE(CLR1_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLR2_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK1, true);
		SIGNAL_WRITE(CLK2, false);
		SIGNAL_WRITE(D1, truth_table[line][0]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(Q1) == truth_table[line][1]);
		munit_assert(SIGNAL_READ_NEXT(Q1_B) == truth_table[line][2]);
		munit_assert_false(SIGNAL_READ_NEXT(Q2));
		munit_assert_true(SIGNAL_READ_NEXT(Q2_B));

		// negative edge
		SIGNAL_WRITE(CLR1_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLR2_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK1, false);
		SIGNAL_WRITE(CLK2, false);
		SIGNAL_WRITE(D1, truth_table[line][0]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(Q1) == truth_table[line][1]);
		munit_assert(SIGNAL_READ_NEXT(Q1_B) == truth_table[line][2]);
		munit_assert_false(SIGNAL_READ_NEXT(Q2));
		munit_assert_true(SIGNAL_READ_NEXT(Q2_B));
	}

	// test flip-flop 2
	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "2: truth_table[%d] - j = %d, k = %d", line, truth_table[line][0], truth_table[line][1]);

		// positive edge
		SIGNAL_WRITE(CLR1_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLR2_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK1, false);
		SIGNAL_WRITE(CLK2, true);
		SIGNAL_WRITE(D2, truth_table[line][0]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(Q2) == truth_table[line][1]);
		munit_assert(SIGNAL_READ_NEXT(Q2_B) == truth_table[line][2]);
		munit_assert_false(SIGNAL_READ_NEXT(Q1));
		munit_assert_true(SIGNAL_READ_NEXT(Q1_B));

		// negative edge
		SIGNAL_WRITE(CLR1_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLR2_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK1, false);
		SIGNAL_WRITE(CLK2, false);
		SIGNAL_WRITE(D2, truth_table[line][0]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(Q2) == truth_table[line][1]);
		munit_assert(SIGNAL_READ_NEXT(Q2_B) == truth_table[line][2]);
		munit_assert_false(SIGNAL_READ_NEXT(Q1));
		munit_assert_true(SIGNAL_READ_NEXT(Q1_B));
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_7493_

static MunitResult test_7493_binary_counter(const MunitParameter params[], void *user_data_or_fixture) {

	Chip7493BinaryCounter *chip = chip_7493_binary_counter_create(simulator_create(NS_TO_PS(100)), (Chip7493Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	SIGNAL(B_B) = SIGNAL(QA);

	// reset counter
	SIGNAL_WRITE(R01, ACTHI_ASSERT);
	SIGNAL_WRITE(R02, ACTHI_ASSERT);
	SIGNAL_WRITE(A_B, true);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_false(SIGNAL_READ_NEXT(QA));
	munit_assert_false(SIGNAL_READ_NEXT(QB));
	munit_assert_false(SIGNAL_READ_NEXT(QC));
	munit_assert_false(SIGNAL_READ_NEXT(QD));

	SIGNAL_WRITE(R01, ACTHI_DEASSERT);
	SIGNAL_WRITE(R02, ACTHI_DEASSERT);

	for (int i = 1; i < 20; ++i) {
		munit_logf(MUNIT_LOG_INFO, "i = %d", i);

		SIGNAL_WRITE(A_B, false);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(QA) == ((i & 0b0001) >> 0));
		munit_assert(SIGNAL_READ_NEXT(QB) == ((i & 0b0010) >> 1));
		munit_assert(SIGNAL_READ_NEXT(QC) == ((i & 0b0100) >> 2));
		munit_assert(SIGNAL_READ_NEXT(QD) == ((i & 0b1000) >> 3));

		SIGNAL_WRITE(A_B, true);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(QA) == ((i & 0b0001) >> 0));
		munit_assert(SIGNAL_READ_NEXT(QB) == ((i & 0b0010) >> 1));
		munit_assert(SIGNAL_READ_NEXT(QC) == ((i & 0b0100) >> 2));
		munit_assert(SIGNAL_READ_NEXT(QD) == ((i & 0b1000) >> 3));
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
};

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74107_

static MunitResult test_74107_jk_flipflop(const MunitParameter params[], void *user_data_or_fixture) {
	Chip74107JKFlipFlop *chip = chip_74107_jk_flipflop_create(simulator_create(NS_TO_PS(100)), (Chip74107Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	// reset both flip-flops
	SIGNAL_WRITE(CLR1_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLR2_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLK1, true);
	SIGNAL_WRITE(CLK2, true);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_false(SIGNAL_READ_NEXT(Q1));
	munit_assert_true(SIGNAL_READ_NEXT(Q1_B));
	munit_assert_false(SIGNAL_READ_NEXT(Q2));
	munit_assert_true(SIGNAL_READ_NEXT(Q2_B));

	bool truth_table[][4] = {
		// j      k      q     q_b
		{false, false, false, true},		// assumes flip-flop was just reset
		{true,  false, true,  false},
		{false, false, true,  false},
		{true , true,  false, true},
		{true , true,  true,  false},
		{false, true,  false, true},
	};

	// test flip-flop 1
	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "1: truth_table[%d] - j = %d, k = %d", line, truth_table[line][0], truth_table[line][1]);

		// negative edge
		SIGNAL_WRITE(CLR1_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLR2_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK1, false);
		SIGNAL_WRITE(CLK2, true);
		SIGNAL_WRITE(J1, truth_table[line][0]);
		SIGNAL_WRITE(K1, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(Q1) == truth_table[line][2]);
		munit_assert(SIGNAL_READ_NEXT(Q1_B) == truth_table[line][3]);
		munit_assert_false(SIGNAL_READ_NEXT(Q2));
		munit_assert_true(SIGNAL_READ_NEXT(Q2_B));

		// positive edge
		SIGNAL_WRITE(CLR1_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLR2_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK1, true);
		SIGNAL_WRITE(CLK2, true);
		SIGNAL_WRITE(J1, truth_table[line][0]);
		SIGNAL_WRITE(K1, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(Q1) == truth_table[line][2]);
		munit_assert(SIGNAL_READ_NEXT(Q1_B) == truth_table[line][3]);
		munit_assert_false(SIGNAL_READ_NEXT(Q2));
		munit_assert_true(SIGNAL_READ_NEXT(Q2_B));
	}

	// test flip-flop 2
	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "2: truth_table[%d] - j = %d, k = %d", line, truth_table[line][0], truth_table[line][1]);

		// negative edge
		SIGNAL_WRITE(CLR1_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLR2_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK1, true);
		SIGNAL_WRITE(CLK2, false);
		SIGNAL_WRITE(J2, truth_table[line][0]);
		SIGNAL_WRITE(K2, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(Q2) == truth_table[line][2]);
		munit_assert(SIGNAL_READ_NEXT(Q2_B) == truth_table[line][3]);
		munit_assert_false(SIGNAL_READ_NEXT(Q1));
		munit_assert_true(SIGNAL_READ_NEXT(Q1_B));

		// positive edge
		SIGNAL_WRITE(CLR1_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLR2_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK1, true);
		SIGNAL_WRITE(CLK2, true);
		SIGNAL_WRITE(J2, truth_table[line][0]);
		SIGNAL_WRITE(K2, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(Q2) == truth_table[line][2]);
		munit_assert(SIGNAL_READ_NEXT(Q2_B) == truth_table[line][3]);
		munit_assert_false(SIGNAL_READ_NEXT(Q1));
		munit_assert_true(SIGNAL_READ_NEXT(Q1_B));
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74145_

static MunitResult test_74145_bcd_decoder(const MunitParameter params[], void *user_data_or_fixture) {
	Chip74145BcdDecoder *chip = chip_74145_bcd_decoder_create(simulator_create(NS_TO_PS(100)), (Chip74145Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	for (int input = 0; input < 16; ++input) {
		SIGNAL_WRITE(A, BIT_IS_SET(input, 0));
		SIGNAL_WRITE(B, BIT_IS_SET(input, 1));
		SIGNAL_WRITE(C, BIT_IS_SET(input, 2));
		SIGNAL_WRITE(D, BIT_IS_SET(input, 3));
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);

		munit_assert(SIGNAL_READ_NEXT(Y0_B) == ((input == 0) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_READ_NEXT(Y1_B) == ((input == 1) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_READ_NEXT(Y2_B) == ((input == 2) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_READ_NEXT(Y3_B) == ((input == 3) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_READ_NEXT(Y4_B) == ((input == 4) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_READ_NEXT(Y5_B) == ((input == 5) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_READ_NEXT(Y6_B) == ((input == 6) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_READ_NEXT(Y7_B) == ((input == 7) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_READ_NEXT(Y8_B) == ((input == 8) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		munit_assert(SIGNAL_READ_NEXT(Y9_B) == ((input == 9) ? ACTLO_ASSERT : ACTLO_DEASSERT));

		signal_pool_cycle(chip->signal_pool, 1);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74153_

static MunitResult test_74153_multiplexer(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74153Multiplexer *chip = chip_74153_multiplexer_create(simulator_create(NS_TO_PS(100)), (Chip74153Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	const bool truth_table[][7] = {
		// B      A     C0     C1     C2     C3     Y
		false, false, false, true,  true,  true,  false,
		false, false, true,  false, false, false, true,
		false, true,  true,  false, true,  true,  false,
		false, true,  false, true,  false, false, true,
		true,  false, true,  true,  false, true,  false,
		true,  false, false, false, true,  false, true,
		true,  true,  true,  true,  true,  false, false,
		true,  true,  false, false, false, true,  true
	};

	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		SIGNAL_WRITE(B, truth_table[line][0]);
		SIGNAL_WRITE(A, truth_table[line][1]);
		SIGNAL_WRITE(C10, truth_table[line][2]);
		SIGNAL_WRITE(C11, truth_table[line][3]);
		SIGNAL_WRITE(C12, truth_table[line][4]);
		SIGNAL_WRITE(C13, truth_table[line][5]);
		SIGNAL_WRITE(C20, truth_table[line][2]);
		SIGNAL_WRITE(C21, truth_table[line][3]);
		SIGNAL_WRITE(C22, truth_table[line][4]);
		SIGNAL_WRITE(C23, truth_table[line][5]);

		// no group activated
		SIGNAL_WRITE(G1, ACTLO_DEASSERT);
		SIGNAL_WRITE(G2, ACTLO_DEASSERT);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert_false(SIGNAL_READ_NEXT(Y1));
		munit_assert_false(SIGNAL_READ_NEXT(Y2));

		// group-1 activated
		SIGNAL_WRITE(G1, ACTLO_ASSERT);
		SIGNAL_WRITE(G2, ACTLO_DEASSERT);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(Y1) == truth_table[line][6]);
		munit_assert_false(SIGNAL_READ_NEXT(Y2));

		// group-2 activated
		SIGNAL_WRITE(G1, ACTLO_DEASSERT);
		SIGNAL_WRITE(G2, ACTLO_ASSERT);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert_false(SIGNAL_READ_NEXT(Y1));
		munit_assert(SIGNAL_READ_NEXT(Y2) == truth_table[line][6]);

		// both groups activated
		SIGNAL_WRITE(G1, ACTLO_ASSERT);
		SIGNAL_WRITE(G2, ACTLO_ASSERT);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(Y1) == truth_table[line][6]);
		munit_assert(SIGNAL_READ_NEXT(Y2) == truth_table[line][6]);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74154_

static MunitResult test_74154_decoder(const MunitParameter params[], void *user_data_or_fixture) {
// TODO: ATM it's not possible to test high impedance output state correctly

	Chip74154Decoder *chip = chip_74154_decoder_create(simulator_create(NS_TO_PS(100)), (Chip74154Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	bool truth_table[4][2] = {
		false, false,
		false, true,
		true,  false,
		true,  true
	};

	for (int i = 0; i < 4; ++i) {
		for (int input = 0; input < 16; ++input) {
			SIGNAL_WRITE(G1_B, truth_table[i][0]);
			SIGNAL_WRITE(G2_B, truth_table[i][1]);
			SIGNAL_WRITE(A, BIT_IS_SET(input, 0));
			SIGNAL_WRITE(B, BIT_IS_SET(input, 1));
			SIGNAL_WRITE(C, BIT_IS_SET(input, 2));
			SIGNAL_WRITE(D, BIT_IS_SET(input, 3));

			signal_pool_cycle(chip->signal_pool, 1);
			chip->process(chip);

			bool strobed = !truth_table[i][0] && !truth_table[i][1];
			munit_assert(SIGNAL_READ_NEXT(Y0_B) == ((strobed && input == 0) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y1_B) == ((strobed && input == 1) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y2_B) == ((strobed && input == 2) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y3_B) == ((strobed && input == 3) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y4_B) == ((strobed && input == 4) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y5_B) == ((strobed && input == 5) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y6_B) == ((strobed && input == 6) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y7_B) == ((strobed && input == 7) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y8_B) == ((strobed && input == 8) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y9_B) == ((strobed && input == 9) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y10_B) == ((strobed && input == 10) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y11_B) == ((strobed && input == 11) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y12_B) == ((strobed && input == 12) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y13_B) == ((strobed && input == 13) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y14_B) == ((strobed && input == 14) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_READ_NEXT(Y15_B) == ((strobed && input == 15) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		}
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74157_

static MunitResult test_74157_multiplexer(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74157Multiplexer *chip = chip_74157_multiplexer_create(simulator_create(NS_TO_PS(100)), (Chip74157Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	// all outputs false when chip is not enabled
	SIGNAL_WRITE(ENABLE_B, ACTLO_DEASSERT);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_false(SIGNAL_READ_NEXT(ZA));
	munit_assert_false(SIGNAL_READ_NEXT(ZB));
	munit_assert_false(SIGNAL_READ_NEXT(ZC));
	munit_assert_false(SIGNAL_READ_NEXT(ZD));

	// check normal operation
	bool truth_table[][4] = {
		//  S    I0     I1    Z
		{true,  true,  false, false},
		{true,  false, true,  true},
		{false, false, true,  false},
		{false, true,  false, true},
	};

	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		SIGNAL_WRITE(ENABLE_B, ACTLO_ASSERT);
		SIGNAL_WRITE(SEL, truth_table[line][0]);
		SIGNAL_WRITE(I0A, truth_table[line][1]);
		SIGNAL_WRITE(I0B, truth_table[line][1]);
		SIGNAL_WRITE(I0C, truth_table[line][1]);
		SIGNAL_WRITE(I0D, truth_table[line][1]);
		SIGNAL_WRITE(I1A, truth_table[line][2]);
		SIGNAL_WRITE(I1B, truth_table[line][2]);
		SIGNAL_WRITE(I1C, truth_table[line][2]);
		SIGNAL_WRITE(I1D, truth_table[line][2]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(ZA) == truth_table[line][3]);
		munit_assert(SIGNAL_READ_NEXT(ZB) == truth_table[line][3]);
		munit_assert(SIGNAL_READ_NEXT(ZC) == truth_table[line][3]);
		munit_assert(SIGNAL_READ_NEXT(ZD) == truth_table[line][3]);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74164_

static MunitResult test_74164_shift_register(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74164ShiftRegister *chip = chip_74164_shift_register_create(simulator_create(NS_TO_PS(100)), (Chip74164Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

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
	SIGNAL_WRITE(CLEAR_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLK, false);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_uint8(chip->state, ==, 0);
	munit_assert_false(SIGNAL_READ_NEXT(QA));
	munit_assert_false(SIGNAL_READ_NEXT(QB));
	munit_assert_false(SIGNAL_READ_NEXT(QC));
	munit_assert_false(SIGNAL_READ_NEXT(QD));
	munit_assert_false(SIGNAL_READ_NEXT(QE));
	munit_assert_false(SIGNAL_READ_NEXT(QF));
	munit_assert_false(SIGNAL_READ_NEXT(QG));
	munit_assert_false(SIGNAL_READ_NEXT(QH));

	// run truth table
	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "truth_table[%d] - a = %d, b = %d", line, truth_table[line][0], truth_table[line][1]);

		// positive edge
		SIGNAL_WRITE(CLEAR_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK, true);
		SIGNAL_WRITE(A, truth_table[line][0]);
		SIGNAL_WRITE(B, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(QA) == truth_table[line][2]);
		munit_assert(SIGNAL_READ_NEXT(QB) == truth_table[line][3]);
		munit_assert(SIGNAL_READ_NEXT(QC) == truth_table[line][4]);
		munit_assert(SIGNAL_READ_NEXT(QD) == truth_table[line][5]);
		munit_assert(SIGNAL_READ_NEXT(QE) == truth_table[line][6]);
		munit_assert(SIGNAL_READ_NEXT(QF) == truth_table[line][7]);
		munit_assert(SIGNAL_READ_NEXT(QG) == truth_table[line][8]);
		munit_assert(SIGNAL_READ_NEXT(QH) == truth_table[line][9]);

		// negative edge
		SIGNAL_WRITE(CLEAR_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK, false);
		SIGNAL_WRITE(A, truth_table[line][0]);
		SIGNAL_WRITE(B, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(QA) == truth_table[line][2]);
		munit_assert(SIGNAL_READ_NEXT(QB) == truth_table[line][3]);
		munit_assert(SIGNAL_READ_NEXT(QC) == truth_table[line][4]);
		munit_assert(SIGNAL_READ_NEXT(QD) == truth_table[line][5]);
		munit_assert(SIGNAL_READ_NEXT(QE) == truth_table[line][6]);
		munit_assert(SIGNAL_READ_NEXT(QF) == truth_table[line][7]);
		munit_assert(SIGNAL_READ_NEXT(QG) == truth_table[line][8]);
		munit_assert(SIGNAL_READ_NEXT(QH) == truth_table[line][9]);
	}

	// reset the shift register again
	SIGNAL_WRITE(CLEAR_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLK, false);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_uint8(chip->state, ==, 0);
	munit_assert_false(SIGNAL_READ_NEXT(QA));
	munit_assert_false(SIGNAL_READ_NEXT(QB));
	munit_assert_false(SIGNAL_READ_NEXT(QC));
	munit_assert_false(SIGNAL_READ_NEXT(QD));
	munit_assert_false(SIGNAL_READ_NEXT(QE));
	munit_assert_false(SIGNAL_READ_NEXT(QF));
	munit_assert_false(SIGNAL_READ_NEXT(QG));
	munit_assert_false(SIGNAL_READ_NEXT(QH));

	simulator_destroy(chip->simulator);
	chip->destroy(chip);

	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74165_

static MunitResult test_74165_shift_register(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74165ShiftRegister *chip = chip_74165_shift_register_create(simulator_create(NS_TO_PS(100)), (Chip74165Signals) {0});
	chip->simulator = simulator_create(NS_TO_PS(100));

	// init signals
	SIGNAL_WRITE(CLK_INH, true);		// inhibit clock
	SIGNAL_WRITE(SI, false);			// shift in zeros
	SIGNAL_WRITE(SL, true);			// shift (not load)
	SIGNAL_WRITE(CLK, false);
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);

	// load shift register
	SIGNAL_WRITE(SL, false);
	SIGNAL_WRITE(A, true);
	SIGNAL_WRITE(B, false);
	SIGNAL_WRITE(C, true);
	SIGNAL_WRITE(D, false);
	SIGNAL_WRITE(E, true);
	SIGNAL_WRITE(F, false);
	SIGNAL_WRITE(G, true);
	SIGNAL_WRITE(H, true);
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);
	munit_assert_int(chip->state, ==, 0b10101011);
	munit_assert_true(SIGNAL_READ_NEXT(QH));
	munit_assert_false(SIGNAL_READ_NEXT(QH_B));
	SIGNAL_WRITE(SL, true);

	// run a few cycles with clock inhibited
	for (int i = 0; i <4; ++i) {
		SIGNAL_WRITE(CLK, !SIGNAL_READ(CLK));
		signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
		chip->simulator->current_tick += 1;
		chip->process(chip);
		munit_assert_int(chip->state, ==, 0b10101011);
		munit_assert_true(SIGNAL_READ_NEXT(QH));
		munit_assert_false(SIGNAL_READ_NEXT(QH_B));
	}
	munit_assert_false(SIGNAL_READ_NEXT(CLK));

	// stop inhiting the clock
	SIGNAL_WRITE(CLK_INH, false);

	// shift the entire byte out
	bool expected[8] = {true, true, false, true, false, true, false, true};

	for (int i = 0; i < 8; ++i) {
		SIGNAL_WRITE(CLK, true);
		signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
		chip->simulator->current_tick += 1;
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(QH) == expected[i]);
		munit_assert(SIGNAL_READ_NEXT(QH_B) != expected[i]);

		SIGNAL_WRITE(CLK, false);
		signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
		chip->simulator->current_tick += 1;
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(QH) == expected[i]);
		munit_assert(SIGNAL_READ_NEXT(QH_B) != expected[i]);
	}

	// test serial input
	SIGNAL_WRITE(SI, !SIGNAL_READ(SI));
	SIGNAL_WRITE(CLK, true);
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);
	munit_assert_int(chip->state, ==, 0b10000000);

	SIGNAL_WRITE(CLK, false);
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);

	SIGNAL_WRITE(SI, !SIGNAL_READ(SI));
	SIGNAL_WRITE(CLK, true);
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);
	munit_assert_int(chip->state, ==, 0b01000000);

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74177_

static MunitResult test_74177_binary_counter(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74177BinaryCounter *chip = chip_74177_binary_counter_create(simulator_create(NS_TO_PS(100)), (Chip74177Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);
	SIGNAL(CLK2) = SIGNAL(QA);

	// reset counter
	SIGNAL_WRITE(CLEAR_B, ACTLO_ASSERT);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_false(SIGNAL_READ_NEXT(QA));
	munit_assert_false(SIGNAL_READ_NEXT(QB));
	munit_assert_false(SIGNAL_READ_NEXT(QC));
	munit_assert_false(SIGNAL_READ_NEXT(QD));

	// preset counter
	SIGNAL_WRITE(LOAD_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLEAR_B, ACTLO_DEASSERT);
	SIGNAL_WRITE(A, true);
	SIGNAL_WRITE(B, false);
	SIGNAL_WRITE(C, true);
	SIGNAL_WRITE(D, false);
	SIGNAL_WRITE(CLK1, true);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_true(SIGNAL_READ_NEXT(QA));
	munit_assert_false(SIGNAL_READ_NEXT(QB));
	munit_assert_true(SIGNAL_READ_NEXT(QC));
	munit_assert_false(SIGNAL_READ_NEXT(QD));
	SIGNAL_WRITE(LOAD_B, ACTLO_DEASSERT);

	for (int i = 6; i < 20; ++i) {
		munit_logf(MUNIT_LOG_INFO, "i = %d", i);

		SIGNAL_WRITE(CLK1, false);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(QA) == ((i & 0b0001) >> 0));
		munit_assert(SIGNAL_READ_NEXT(QB) == ((i & 0b0010) >> 1));
		munit_assert(SIGNAL_READ_NEXT(QC) == ((i & 0b0100) >> 2));
		munit_assert(SIGNAL_READ_NEXT(QD) == ((i & 0b1000) >> 3));

		SIGNAL_WRITE(CLK1, true);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(QA) == ((i & 0b0001) >> 0));
		munit_assert(SIGNAL_READ_NEXT(QB) == ((i & 0b0010) >> 1));
		munit_assert(SIGNAL_READ_NEXT(QC) == ((i & 0b0100) >> 2));
		munit_assert(SIGNAL_READ_NEXT(QD) == ((i & 0b1000) >> 3));
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
};

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74191_

static MunitResult test_74191_binary_counter(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74191BinaryCounter *chip = chip_74191_binary_counter_create(simulator_create(NS_TO_PS(100)), (Chip74191Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	// initialize with 'random' value
	chip->state = 0xfa;

	// load the counter
	SIGNAL_WRITE(LOAD_B, ACTLO_ASSERT);
	SIGNAL_WRITE(CLK, false);
	SIGNAL_WRITE(A, true);
	SIGNAL_WRITE(B, false);
	SIGNAL_WRITE(C, true);
	SIGNAL_WRITE(D, true);
	signal_pool_cycle(chip->signal_pool, 1);
	chip->process(chip);
	munit_assert_uint8(chip->state, ==, 13);

	bool truth_table[][8] = {		// replicates timing diagram from datasheet
		// du     en     qa     qb     qc     qd  min/max  rco
		{false, false, false, true,  true,  true,  false, true},	// 1
		{false, false, true,  true,  true,  true,  true,  false},	// 2
		{false, false, false, false, false, false, false, true},	// 3
		{false, false, true,  false, false, false, false, true},	// 4
		{false, false, false, true,  false, false, false, true},	// 5
		{false, true,  false, true,  false, false, false, true},	// 6
		{true,  true,  false, true,  false, false, false, true},	// 7
		{true,  false, true,  false, false, false, false, true},	// 8
		{true,  false, false, false, false, false, true,  false},	// 9
		{true,  false, true,  true,  true,  true,  false, true},	// 10
		{true,  false, false, true,  true,  true,  false, true},	// 11
		{true,  false, true,  false, true,  true,  false, true},	// 12
	};

	// run truth table
	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "truth_table[%d] - a = %d, b = %d", line, truth_table[line][0], truth_table[line][1]);

		// positive edge
		SIGNAL_WRITE(LOAD_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK, true);
		SIGNAL_WRITE(D_U, truth_table[line][0]);
		SIGNAL_WRITE(ENABLE_B, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(QA) == truth_table[line][2]);
		munit_assert(SIGNAL_READ_NEXT(QB) == truth_table[line][3]);
		munit_assert(SIGNAL_READ_NEXT(QC) == truth_table[line][4]);
		munit_assert(SIGNAL_READ_NEXT(QD) == truth_table[line][5]);
		munit_assert(SIGNAL_READ_NEXT(MAX_MIN) == truth_table[line][6]);
		munit_assert_true(SIGNAL_READ_NEXT(RCO_B));

		// negative edge
		SIGNAL_WRITE(LOAD_B, ACTLO_DEASSERT);
		SIGNAL_WRITE(CLK, false);
		SIGNAL_WRITE(D_U, truth_table[line][0]);
		SIGNAL_WRITE(ENABLE_B, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_READ_NEXT(QA) == truth_table[line][2]);
		munit_assert(SIGNAL_READ_NEXT(QB) == truth_table[line][3]);
		munit_assert(SIGNAL_READ_NEXT(QC) == truth_table[line][4]);
		munit_assert(SIGNAL_READ_NEXT(QD) == truth_table[line][5]);
		munit_assert(SIGNAL_READ_NEXT(MAX_MIN) == truth_table[line][6]);
		munit_assert(SIGNAL_READ_NEXT(RCO_B) == truth_table[line][7]);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74244_

static MunitResult test_74244_octal_buffer(const MunitParameter params[], void *user_data_or_fixture) {
// TODO: ATM it's not possible to test high impedance output state correctly

	bool truth_table[4][2] = {
		false, false,
		false, true,
		true,  false,
		true,  true
	};

	Chip74244OctalBuffer *chip = chip_74244_octal_buffer_create(simulator_create(NS_TO_PS(100)), (Chip74244Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	for (int i = 0; i < 4; ++i) {

		for (int input = 0; input < 256; ++input) {
			SIGNAL_WRITE(G1_B, truth_table[i][0]);
			SIGNAL_WRITE(G2_B, truth_table[i][1]);

			SIGNAL_WRITE(A11, BIT_IS_SET(input, 0));
			SIGNAL_WRITE(A12, BIT_IS_SET(input, 1));
			SIGNAL_WRITE(A13, BIT_IS_SET(input, 2));
			SIGNAL_WRITE(A14, BIT_IS_SET(input, 3));
			SIGNAL_WRITE(A21, BIT_IS_SET(input, 4));
			SIGNAL_WRITE(A22, BIT_IS_SET(input, 5));
			SIGNAL_WRITE(A23, BIT_IS_SET(input, 6));
			SIGNAL_WRITE(A24, BIT_IS_SET(input, 7));

			signal_pool_cycle(chip->signal_pool, 1);
			chip->process(chip);

			munit_assert(SIGNAL_READ_NEXT(Y11) == (truth_table[i][0] ? false : BIT_IS_SET(input, 0)));
			munit_assert(SIGNAL_READ_NEXT(Y12) == (truth_table[i][0] ? false : BIT_IS_SET(input, 1)));
			munit_assert(SIGNAL_READ_NEXT(Y13) == (truth_table[i][0] ? false : BIT_IS_SET(input, 2)));
			munit_assert(SIGNAL_READ_NEXT(Y14) == (truth_table[i][0] ? false : BIT_IS_SET(input, 3)));
			munit_assert(SIGNAL_READ_NEXT(Y21) == (truth_table[i][1] ? false : BIT_IS_SET(input, 4)));
			munit_assert(SIGNAL_READ_NEXT(Y22) == (truth_table[i][1] ? false : BIT_IS_SET(input, 5)));
			munit_assert(SIGNAL_READ_NEXT(Y23) == (truth_table[i][1] ? false : BIT_IS_SET(input, 6)));
			munit_assert(SIGNAL_READ_NEXT(Y24) == (truth_table[i][1] ? false : BIT_IS_SET(input, 7)));
		}
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);

	return MUNIT_OK;
}

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74373_

static MunitResult test_74373_latch(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74373Latch *chip = chip_74373_latch_create(simulator_create(NS_TO_PS(100)), (Chip74373Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_not_null(chip->process);
	munit_assert_ptr_not_null(chip->register_dependencies);
	munit_assert_ptr_not_null(chip->destroy);

	bool truth_table[][18] = {
		//  oc    c      d1     d2     d3     d4     d5     d6     d7     d8     q1     q2     q3     q4     q5     q6     q7     q8
		{false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true},
		{false, true,  true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  false},
		{false, false, false, false, false, false, false, false, false, false, true,  false, true,  false, true,  false, true,  false}
	};

	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "truth_table[%d]", line);

		SIGNAL_WRITE(OC_B, truth_table[line][0]);
		SIGNAL_WRITE(C, truth_table[line][1]);
		SIGNAL_WRITE(D1, truth_table[line][2]);
		SIGNAL_WRITE(D2, truth_table[line][3]);
		SIGNAL_WRITE(D3, truth_table[line][4]);
		SIGNAL_WRITE(D4, truth_table[line][5]);
		SIGNAL_WRITE(D5, truth_table[line][6]);
		SIGNAL_WRITE(D6, truth_table[line][7]);
		SIGNAL_WRITE(D7, truth_table[line][8]);
		SIGNAL_WRITE(D8, truth_table[line][9]);

		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);

		munit_assert(SIGNAL_READ_NEXT(Q1) == truth_table[line][10]);
		munit_assert(SIGNAL_READ_NEXT(Q2) == truth_table[line][11]);
		munit_assert(SIGNAL_READ_NEXT(Q3) == truth_table[line][12]);
		munit_assert(SIGNAL_READ_NEXT(Q4) == truth_table[line][13]);
		munit_assert(SIGNAL_READ_NEXT(Q5) == truth_table[line][14]);
		munit_assert(SIGNAL_READ_NEXT(Q6) == truth_table[line][15]);
		munit_assert(SIGNAL_READ_NEXT(Q7) == truth_table[line][16]);
		munit_assert(SIGNAL_READ_NEXT(Q8) == truth_table[line][17]);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

MunitTest chip_74xxx_tests[] = {
	{ "/7400_nand", test_7400_nand, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/7474_d_flipflop", test_7474_d_flipflop, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/7493_binary_counter", test_7493_binary_counter, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74107_jk_flipflop", test_74107_jk_flipflop, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74145_bcd_decoder", test_74145_bcd_decoder, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74153_multiplexer", test_74153_multiplexer, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74154_decoder", test_74154_decoder, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74157_multiplexer", test_74157_multiplexer, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74164_shift_register", test_74164_shift_register, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74165_shift_register", test_74165_shift_register, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74177_binary_counter", test_74177_binary_counter, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74191_binary_counter", test_74191_binary_counter, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74244_octal_buffer", test_74244_octal_buffer, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/74373_latch", test_74373_latch, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
