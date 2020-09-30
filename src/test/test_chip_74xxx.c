// test/test_chip_74xxx.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_74xxx.h"
#include "simulator.h"

#define SIGNAL_POOL			chip->signal_pool
#define SIGNAL_COLLECTION	chip->signals
#define SIGNAL_CHIP_ID		chip->id

static MunitResult test_7400_nand(const MunitParameter params[], void *user_data_or_fixture) {

	bool truth_table[4][3] = {
		false, false, true,
		false, true,  true,
		true,  false, true,
		true,  true,  false
	};

	Chip7400Nand *chip = chip_7400_nand_create(simulator_create(NS_TO_PS(100)), (Chip7400NandSignals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_7400_nand_process);
	munit_assert_ptr_equal(chip->destroy, chip_7400_nand_destroy);

	for (int i = 0; i < 4; ++i) {
		SIGNAL_SET_BOOL(a1, truth_table[(i+0)%4][0]);
		SIGNAL_SET_BOOL(b1, truth_table[(i+0)%4][1]);
		SIGNAL_SET_BOOL(a2, truth_table[(i+1)%4][0]);
		SIGNAL_SET_BOOL(b2, truth_table[(i+1)%4][1]);
		SIGNAL_SET_BOOL(a3, truth_table[(i+2)%4][0]);
		SIGNAL_SET_BOOL(b3, truth_table[(i+2)%4][1]);
		SIGNAL_SET_BOOL(a4, truth_table[(i+3)%4][0]);
		SIGNAL_SET_BOOL(b4, truth_table[(i+3)%4][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_7400_nand_process(chip);

		munit_assert(SIGNAL_NEXT_BOOL(y1) == truth_table[(i+0)%4][2]);
		munit_assert(SIGNAL_NEXT_BOOL(y2) == truth_table[(i+1)%4][2]);
		munit_assert(SIGNAL_NEXT_BOOL(y3) == truth_table[(i+2)%4][2]);
		munit_assert(SIGNAL_NEXT_BOOL(y4) == truth_table[(i+3)%4][2]);
	}

	simulator_destroy(chip->simulator);
	chip_7400_nand_destroy(chip);
	return MUNIT_OK;
}

static MunitResult test_7474_d_flipflop(const MunitParameter params[], void *user_data_or_fixture) {
	Chip7474DFlipFlop *chip = chip_7474_d_flipflop_create(simulator_create(NS_TO_PS(100)), (Chip7474Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_7474_d_flipflop_process);
	munit_assert_ptr_equal(chip->destroy, chip_7474_d_flipflop_destroy);

	// set both flip-flops
	SIGNAL_SET_BOOL(pr1_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(pr2_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clr1_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(clr2_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(clk1, false);
	SIGNAL_SET_BOOL(clk2, false);
	signal_pool_cycle(chip->signal_pool, 1);
	chip_7474_d_flipflop_process(chip);
	munit_assert_true(SIGNAL_NEXT_BOOL(q1));
	munit_assert_false(SIGNAL_NEXT_BOOL(q1_b));
	munit_assert_true(SIGNAL_NEXT_BOOL(q2));
	munit_assert_false(SIGNAL_NEXT_BOOL(q2_b));

	// check nonstable state when both clear and preset are asserted
	SIGNAL_SET_BOOL(pr1_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(pr2_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clr1_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clr2_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clk1, false);
	SIGNAL_SET_BOOL(clk2, false);
	signal_pool_cycle(chip->signal_pool, 1);
	chip_7474_d_flipflop_process(chip);
	munit_assert_true(SIGNAL_NEXT_BOOL(q1));
	munit_assert_true(SIGNAL_NEXT_BOOL(q1_b));
	munit_assert_true(SIGNAL_NEXT_BOOL(q2));
	munit_assert_true(SIGNAL_NEXT_BOOL(q2_b));

	// clear both flip-flops
	SIGNAL_SET_BOOL(pr1_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(pr2_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(clr1_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clr2_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clk1, false);
	SIGNAL_SET_BOOL(clk2, false);
	signal_pool_cycle(chip->signal_pool, 1);
	chip_7474_d_flipflop_process(chip);
	munit_assert_false(SIGNAL_NEXT_BOOL(q1));
	munit_assert_true(SIGNAL_NEXT_BOOL(q1_b));
	munit_assert_false(SIGNAL_NEXT_BOOL(q2));
	munit_assert_true(SIGNAL_NEXT_BOOL(q2_b));

	bool truth_table[][3] = {
		// d      q     q_b
		{true,  true,  false},
		{false, false, true}
	};

	// test flip-flop 1
	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "1: truth_table[%d] - d = %d", line, truth_table[line][0]);

		// positive edge
		SIGNAL_SET_BOOL(clr1_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clr2_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk1, true);
		SIGNAL_SET_BOOL(clk2, false);
		SIGNAL_SET_BOOL(d1, truth_table[line][0]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_7474_d_flipflop_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(q1) == truth_table[line][1]);
		munit_assert(SIGNAL_NEXT_BOOL(q1_b) == truth_table[line][2]);
		munit_assert_false(SIGNAL_NEXT_BOOL(q2));
		munit_assert_true(SIGNAL_NEXT_BOOL(q2_b));

		// negative edge
		SIGNAL_SET_BOOL(clr1_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clr2_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk1, false);
		SIGNAL_SET_BOOL(clk2, false);
		SIGNAL_SET_BOOL(d1, truth_table[line][0]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_7474_d_flipflop_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(q1) == truth_table[line][1]);
		munit_assert(SIGNAL_NEXT_BOOL(q1_b) == truth_table[line][2]);
		munit_assert_false(SIGNAL_NEXT_BOOL(q2));
		munit_assert_true(SIGNAL_NEXT_BOOL(q2_b));
	}

	// test flip-flop 2
	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "2: truth_table[%d] - j = %d, k = %d", line, truth_table[line][0], truth_table[line][1]);

		// positive edge
		SIGNAL_SET_BOOL(clr1_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clr2_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk1, false);
		SIGNAL_SET_BOOL(clk2, true);
		SIGNAL_SET_BOOL(d2, truth_table[line][0]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_7474_d_flipflop_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(q2) == truth_table[line][1]);
		munit_assert(SIGNAL_NEXT_BOOL(q2_b) == truth_table[line][2]);
		munit_assert_false(SIGNAL_NEXT_BOOL(q1));
		munit_assert_true(SIGNAL_NEXT_BOOL(q1_b));

		// negative edge
		SIGNAL_SET_BOOL(clr1_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clr2_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk1, false);
		SIGNAL_SET_BOOL(clk2, false);
		SIGNAL_SET_BOOL(d2, truth_table[line][0]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_7474_d_flipflop_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(q2) == truth_table[line][1]);
		munit_assert(SIGNAL_NEXT_BOOL(q2_b) == truth_table[line][2]);
		munit_assert_false(SIGNAL_NEXT_BOOL(q1));
		munit_assert_true(SIGNAL_NEXT_BOOL(q1_b));
	}

	simulator_destroy(chip->simulator);
	chip_7474_d_flipflop_destroy(chip);
	return MUNIT_OK;
}

static MunitResult test_7493_binary_counter(const MunitParameter params[], void *user_data_or_fixture) {

	Chip7493BinaryCounter *chip = chip_7493_binary_counter_create(simulator_create(NS_TO_PS(100)), (Chip7493Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_7493_binary_counter_process);
	munit_assert_ptr_equal(chip->destroy, chip_7493_binary_counter_destroy);

	chip->signals.b_b = chip->signals.qa;

	// reset counter
	SIGNAL_SET_BOOL(r01, ACTHI_ASSERT);
	SIGNAL_SET_BOOL(r02, ACTHI_ASSERT);
	SIGNAL_SET_BOOL(a_b, true);
	signal_pool_cycle(chip->signal_pool, 1);
	chip_7493_binary_counter_process(chip);
	munit_assert_false(SIGNAL_NEXT_BOOL(qa));
	munit_assert_false(SIGNAL_NEXT_BOOL(qb));
	munit_assert_false(SIGNAL_NEXT_BOOL(qc));
	munit_assert_false(SIGNAL_NEXT_BOOL(qd));

	SIGNAL_SET_BOOL(r01, ACTHI_DEASSERT);
	SIGNAL_SET_BOOL(r02, ACTHI_DEASSERT);

	for (int i = 1; i < 20; ++i) {
		munit_logf(MUNIT_LOG_INFO, "i = %d", i);

		SIGNAL_SET_BOOL(a_b, false);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_7493_binary_counter_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(qa) == ((i & 0b0001) >> 0));
		munit_assert(SIGNAL_NEXT_BOOL(qb) == ((i & 0b0010) >> 1));
		munit_assert(SIGNAL_NEXT_BOOL(qc) == ((i & 0b0100) >> 2));
		munit_assert(SIGNAL_NEXT_BOOL(qd) == ((i & 0b1000) >> 3));

		SIGNAL_SET_BOOL(a_b, true);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_7493_binary_counter_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(qa) == ((i & 0b0001) >> 0));
		munit_assert(SIGNAL_NEXT_BOOL(qb) == ((i & 0b0010) >> 1));
		munit_assert(SIGNAL_NEXT_BOOL(qc) == ((i & 0b0100) >> 2));
		munit_assert(SIGNAL_NEXT_BOOL(qd) == ((i & 0b1000) >> 3));
	}

	simulator_destroy(chip->simulator);
	chip_7493_binary_counter_destroy(chip);
	return MUNIT_OK;
};

static MunitResult test_74107_jk_flipflop(const MunitParameter params[], void *user_data_or_fixture) {
	Chip74107JKFlipFlop *chip = chip_74107_jk_flipflop_create(simulator_create(NS_TO_PS(100)), (Chip74107Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_74107_jk_flipflop_process);
	munit_assert_ptr_equal(chip->destroy, chip_74107_jk_flipflop_destroy);

	// reset both flip-flops
	SIGNAL_SET_BOOL(clr1_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clr2_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clk1, true);
	SIGNAL_SET_BOOL(clk2, true);
	signal_pool_cycle(chip->signal_pool, 1);
	chip_74107_jk_flipflop_process(chip);
	munit_assert_false(SIGNAL_NEXT_BOOL(q1));
	munit_assert_true(SIGNAL_NEXT_BOOL(q1_b));
	munit_assert_false(SIGNAL_NEXT_BOOL(q2));
	munit_assert_true(SIGNAL_NEXT_BOOL(q2_b));

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
		SIGNAL_SET_BOOL(clr1_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clr2_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk1, false);
		SIGNAL_SET_BOOL(clk2, true);
		SIGNAL_SET_BOOL(j1, truth_table[line][0]);
		SIGNAL_SET_BOOL(k1, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_74107_jk_flipflop_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(q1) == truth_table[line][2]);
		munit_assert(SIGNAL_NEXT_BOOL(q1_b) == truth_table[line][3]);
		munit_assert_false(SIGNAL_NEXT_BOOL(q2));
		munit_assert_true(SIGNAL_NEXT_BOOL(q2_b));

		// positive edge
		SIGNAL_SET_BOOL(clr1_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clr2_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk1, true);
		SIGNAL_SET_BOOL(clk2, true);
		SIGNAL_SET_BOOL(j1, truth_table[line][0]);
		SIGNAL_SET_BOOL(k1, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_74107_jk_flipflop_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(q1) == truth_table[line][2]);
		munit_assert(SIGNAL_NEXT_BOOL(q1_b) == truth_table[line][3]);
		munit_assert_false(SIGNAL_NEXT_BOOL(q2));
		munit_assert_true(SIGNAL_NEXT_BOOL(q2_b));
	}

	// test flip-flop 2
	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "2: truth_table[%d] - j = %d, k = %d", line, truth_table[line][0], truth_table[line][1]);

		// negative edge
		SIGNAL_SET_BOOL(clr1_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clr2_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk1, true);
		SIGNAL_SET_BOOL(clk2, false);
		SIGNAL_SET_BOOL(j2, truth_table[line][0]);
		SIGNAL_SET_BOOL(k2, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_74107_jk_flipflop_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(q2) == truth_table[line][2]);
		munit_assert(SIGNAL_NEXT_BOOL(q2_b) == truth_table[line][3]);
		munit_assert_false(SIGNAL_NEXT_BOOL(q1));
		munit_assert_true(SIGNAL_NEXT_BOOL(q1_b));

		// positive edge
		SIGNAL_SET_BOOL(clr1_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clr2_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk1, true);
		SIGNAL_SET_BOOL(clk2, true);
		SIGNAL_SET_BOOL(j2, truth_table[line][0]);
		SIGNAL_SET_BOOL(k2, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_74107_jk_flipflop_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(q2) == truth_table[line][2]);
		munit_assert(SIGNAL_NEXT_BOOL(q2_b) == truth_table[line][3]);
		munit_assert_false(SIGNAL_NEXT_BOOL(q1));
		munit_assert_true(SIGNAL_NEXT_BOOL(q1_b));
	}

	simulator_destroy(chip->simulator);
	chip_74107_jk_flipflop_destroy(chip);
	return MUNIT_OK;
}

static MunitResult test_74145_bcd_decoder(const MunitParameter params[], void *user_data_or_fixture) {
	Chip74145BcdDecoder *chip = chip_74145_bcd_decoder_create(simulator_create(NS_TO_PS(100)), (Chip74145Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_74145_bcd_decoder_process);
	munit_assert_ptr_equal(chip->destroy, chip_74145_bcd_decoder_destroy);

	for (int input = 0; input < 16; ++input) {
		SIGNAL_SET_BOOL(a, BIT_IS_SET(input, 0));
		SIGNAL_SET_BOOL(b, BIT_IS_SET(input, 1));
		SIGNAL_SET_BOOL(c, BIT_IS_SET(input, 2));
		SIGNAL_SET_BOOL(d, BIT_IS_SET(input, 3));
		signal_pool_cycle(chip->signal_pool, 1);
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

		signal_pool_cycle(chip->signal_pool, 1);
	}

	simulator_destroy(chip->simulator);
	chip_74145_bcd_decoder_destroy(chip);
	return MUNIT_OK;
}

static MunitResult test_74153_multiplexer(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74153Multiplexer *chip = chip_74153_multiplexer_create(simulator_create(NS_TO_PS(100)), (Chip74153Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_74153_multiplexer_process);
	munit_assert_ptr_equal(chip->register_dependencies, chip_74153_multiplexer_register_dependencies);
	munit_assert_ptr_equal(chip->destroy, chip_74153_multiplexer_destroy);

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
		SIGNAL_SET_BOOL(b, truth_table[line][0]);
		SIGNAL_SET_BOOL(a, truth_table[line][1]);
		SIGNAL_SET_BOOL(c10, truth_table[line][2]);
		SIGNAL_SET_BOOL(c11, truth_table[line][3]);
		SIGNAL_SET_BOOL(c12, truth_table[line][4]);
		SIGNAL_SET_BOOL(c13, truth_table[line][5]);
		SIGNAL_SET_BOOL(c20, truth_table[line][2]);
		SIGNAL_SET_BOOL(c21, truth_table[line][3]);
		SIGNAL_SET_BOOL(c22, truth_table[line][4]);
		SIGNAL_SET_BOOL(c23, truth_table[line][5]);

		// no group activated
		SIGNAL_SET_BOOL(g1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(g2, ACTLO_DEASSERT);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert_false(SIGNAL_NEXT_BOOL(y1));
		munit_assert_false(SIGNAL_NEXT_BOOL(y2));

		// group-1 activated
		SIGNAL_SET_BOOL(g1, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(g2, ACTLO_DEASSERT);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(y1) == truth_table[line][6]);
		munit_assert_false(SIGNAL_NEXT_BOOL(y2));

		// group-2 activated
		SIGNAL_SET_BOOL(g1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(g2, ACTLO_ASSERT);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert_false(SIGNAL_NEXT_BOOL(y1));
		munit_assert(SIGNAL_NEXT_BOOL(y2) == truth_table[line][6]);

		// both groups activated
		SIGNAL_SET_BOOL(g1, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(g2, ACTLO_ASSERT);
		signal_pool_cycle(chip->signal_pool, 1);
		chip->process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(y1) == truth_table[line][6]);
		munit_assert(SIGNAL_NEXT_BOOL(y2) == truth_table[line][6]);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);
	return MUNIT_OK;
}

static MunitResult test_74154_decoder(const MunitParameter params[], void *user_data_or_fixture) {
// TODO: ATM it's not possible to test high impedance output state correctly

	Chip74154Decoder *chip = chip_74154_decoder_create(simulator_create(NS_TO_PS(100)), (Chip74154Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_74154_decoder_process);
	munit_assert_ptr_equal(chip->destroy, chip_74154_decoder_destroy);

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

			signal_pool_cycle(chip->signal_pool, 1);
			chip_74154_decoder_process(chip);

			bool strobed = !truth_table[i][0] && !truth_table[i][1];
			munit_assert(SIGNAL_NEXT_BOOL(y0_b) == ((strobed && input == 0) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y1_b) == ((strobed && input == 1) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y2_b) == ((strobed && input == 2) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y3_b) == ((strobed && input == 3) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y4_b) == ((strobed && input == 4) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y5_b) == ((strobed && input == 5) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y6_b) == ((strobed && input == 6) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y7_b) == ((strobed && input == 7) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y8_b) == ((strobed && input == 8) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y9_b) == ((strobed && input == 9) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y10_b) == ((strobed && input == 10) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y11_b) == ((strobed && input == 11) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y12_b) == ((strobed && input == 12) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y13_b) == ((strobed && input == 13) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y14_b) == ((strobed && input == 14) ? ACTLO_ASSERT : ACTLO_DEASSERT));
			munit_assert(SIGNAL_NEXT_BOOL(y15_b) == ((strobed && input == 15) ? ACTLO_ASSERT : ACTLO_DEASSERT));
		}
	}

	simulator_destroy(chip->simulator);
	chip_74154_decoder_destroy(chip);
	return MUNIT_OK;
}

static MunitResult test_74157_multiplexer(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74157Multiplexer *chip = chip_74157_multiplexer_create(simulator_create(NS_TO_PS(100)), (Chip74157Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_74157_multiplexer_process);
	munit_assert_ptr_equal(chip->destroy, chip_74157_multiplexer_destroy);

	// all outputs false when chip is not enabled
	SIGNAL_SET_BOOL(enable_b, ACTLO_DEASSERT);
	signal_pool_cycle(chip->signal_pool, 1);
	chip_74157_multiplexer_process(chip);
	munit_assert_false(SIGNAL_NEXT_BOOL(za));
	munit_assert_false(SIGNAL_NEXT_BOOL(zb));
	munit_assert_false(SIGNAL_NEXT_BOOL(zc));
	munit_assert_false(SIGNAL_NEXT_BOOL(zd));

	// check normal operation
	bool truth_table[][4] = {
		//  S    I0     I1    Z
		{true,  true,  false, false},
		{true,  false, true,  true},
		{false, false, true,  false},
		{false, true,  false, true},
	};

	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		SIGNAL_SET_BOOL(enable_b, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(sel, truth_table[line][0]);
		SIGNAL_SET_BOOL(i0a, truth_table[line][1]);
		SIGNAL_SET_BOOL(i0b, truth_table[line][1]);
		SIGNAL_SET_BOOL(i0c, truth_table[line][1]);
		SIGNAL_SET_BOOL(i0d, truth_table[line][1]);
		SIGNAL_SET_BOOL(i1a, truth_table[line][2]);
		SIGNAL_SET_BOOL(i1b, truth_table[line][2]);
		SIGNAL_SET_BOOL(i1c, truth_table[line][2]);
		SIGNAL_SET_BOOL(i1d, truth_table[line][2]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_74157_multiplexer_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(za) == truth_table[line][3]);
		munit_assert(SIGNAL_NEXT_BOOL(zb) == truth_table[line][3]);
		munit_assert(SIGNAL_NEXT_BOOL(zc) == truth_table[line][3]);
		munit_assert(SIGNAL_NEXT_BOOL(zd) == truth_table[line][3]);
	}

	simulator_destroy(chip->simulator);
	chip_74157_multiplexer_destroy(chip);
	return MUNIT_OK;
}

static MunitResult test_74164_shift_register(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74164ShiftRegister *chip = chip_74164_shift_register_create(simulator_create(NS_TO_PS(100)), (Chip74164Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_74164_shift_register_process);
	munit_assert_ptr_equal(chip->destroy, chip_74164_shift_register_destroy);

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
	signal_pool_cycle(chip->signal_pool, 1);
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
		signal_pool_cycle(chip->signal_pool, 1);
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
		signal_pool_cycle(chip->signal_pool, 1);
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
	signal_pool_cycle(chip->signal_pool, 1);
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

	simulator_destroy(chip->simulator);
	chip_74164_shift_register_destroy(chip);

	return MUNIT_OK;
}

static MunitResult test_74165_shift_register(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74165ShiftRegister *chip = chip_74165_shift_register_create(simulator_create(NS_TO_PS(100)), (Chip74165Signals) {0});
	chip->simulator = simulator_create(NS_TO_PS(100));

	// init signals
	SIGNAL_SET_BOOL(clk_inh, true);		// inhibit clock
	SIGNAL_SET_BOOL(si, false);			// shift in zeros
	SIGNAL_SET_BOOL(sl, true);			// shift (not load)
	SIGNAL_SET_BOOL(clk, false);
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);

	// load shift register
	SIGNAL_SET_BOOL(sl, false);
	SIGNAL_SET_BOOL(a, true);
	SIGNAL_SET_BOOL(b, false);
	SIGNAL_SET_BOOL(c, true);
	SIGNAL_SET_BOOL(d, false);
	SIGNAL_SET_BOOL(e, true);
	SIGNAL_SET_BOOL(f, false);
	SIGNAL_SET_BOOL(g, true);
	SIGNAL_SET_BOOL(h, true);
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);
	munit_assert_int(chip->state, ==, 0b10101011);
	munit_assert_true(SIGNAL_NEXT_BOOL(qh));
	munit_assert_false(SIGNAL_NEXT_BOOL(qh_b));
	SIGNAL_SET_BOOL(sl, true);

	// run a few cycles with clock inhibited
	for (int i = 0; i <4; ++i) {
		SIGNAL_SET_BOOL(clk, !SIGNAL_BOOL(clk));
		signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
		chip->simulator->current_tick += 1;
		chip->process(chip);
		munit_assert_int(chip->state, ==, 0b10101011);
		munit_assert_true(SIGNAL_NEXT_BOOL(qh));
		munit_assert_false(SIGNAL_NEXT_BOOL(qh_b));
	}
	munit_assert_false(SIGNAL_NEXT_BOOL(clk));

	// stop inhiting the clock
	SIGNAL_SET_BOOL(clk_inh, false);

	// shift the entire byte out
	bool expected[8] = {true, true, false, true, false, true, false, true};

	for (int i = 0; i < 8; ++i) {
		SIGNAL_SET_BOOL(clk, true);
		signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
		chip->simulator->current_tick += 1;
		chip->process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(qh) == expected[i]);
		munit_assert(SIGNAL_NEXT_BOOL(qh_b) != expected[i]);

		SIGNAL_SET_BOOL(clk, false);
		signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
		chip->simulator->current_tick += 1;
		chip->process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(qh) == expected[i]);
		munit_assert(SIGNAL_NEXT_BOOL(qh_b) != expected[i]);
	}

	// test serial input
	SIGNAL_SET_BOOL(si, !SIGNAL_BOOL(si));
	SIGNAL_SET_BOOL(clk, true);
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);
	munit_assert_int(chip->state, ==, 0b10000000);

	SIGNAL_SET_BOOL(clk, false);
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);

	SIGNAL_SET_BOOL(si, !SIGNAL_BOOL(si));
	SIGNAL_SET_BOOL(clk, true);
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);
	munit_assert_int(chip->state, ==, 0b01000000);

	simulator_destroy(chip->simulator);
	chip_74165_shift_register_destroy(chip);
	return MUNIT_OK;
}

static MunitResult test_74177_binary_counter(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74177BinaryCounter *chip = chip_74177_binary_counter_create(simulator_create(NS_TO_PS(100)), (Chip74177Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_74177_binary_counter_process);
	munit_assert_ptr_equal(chip->destroy, chip_74177_binary_counter_destroy);
	chip->signals.clk2 = chip->signals.qa;

	// reset counter
	SIGNAL_SET_BOOL(clear_b, ACTLO_ASSERT);
	signal_pool_cycle(chip->signal_pool, 1);
	chip_74177_binary_counter_process(chip);
	munit_assert_false(SIGNAL_NEXT_BOOL(qa));
	munit_assert_false(SIGNAL_NEXT_BOOL(qb));
	munit_assert_false(SIGNAL_NEXT_BOOL(qc));
	munit_assert_false(SIGNAL_NEXT_BOOL(qd));

	// preset counter
	SIGNAL_SET_BOOL(load_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clear_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(a, true);
	SIGNAL_SET_BOOL(b, false);
	SIGNAL_SET_BOOL(c, true);
	SIGNAL_SET_BOOL(d, false);
	SIGNAL_SET_BOOL(clk1, true);
	signal_pool_cycle(chip->signal_pool, 1);
	chip_74177_binary_counter_process(chip);
	munit_assert_true(SIGNAL_NEXT_BOOL(qa));
	munit_assert_false(SIGNAL_NEXT_BOOL(qb));
	munit_assert_true(SIGNAL_NEXT_BOOL(qc));
	munit_assert_false(SIGNAL_NEXT_BOOL(qd));
	SIGNAL_SET_BOOL(load_b, ACTLO_DEASSERT);

	for (int i = 6; i < 20; ++i) {
		munit_logf(MUNIT_LOG_INFO, "i = %d", i);

		SIGNAL_SET_BOOL(clk1, false);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_74177_binary_counter_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(qa) == ((i & 0b0001) >> 0));
		munit_assert(SIGNAL_NEXT_BOOL(qb) == ((i & 0b0010) >> 1));
		munit_assert(SIGNAL_NEXT_BOOL(qc) == ((i & 0b0100) >> 2));
		munit_assert(SIGNAL_NEXT_BOOL(qd) == ((i & 0b1000) >> 3));

		SIGNAL_SET_BOOL(clk1, true);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_74177_binary_counter_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(qa) == ((i & 0b0001) >> 0));
		munit_assert(SIGNAL_NEXT_BOOL(qb) == ((i & 0b0010) >> 1));
		munit_assert(SIGNAL_NEXT_BOOL(qc) == ((i & 0b0100) >> 2));
		munit_assert(SIGNAL_NEXT_BOOL(qd) == ((i & 0b1000) >> 3));
	}

	simulator_destroy(chip->simulator);
	chip_74177_binary_counter_destroy(chip);
	return MUNIT_OK;
};

static MunitResult test_74191_binary_counter(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74191BinaryCounter *chip = chip_74191_binary_counter_create(simulator_create(NS_TO_PS(100)), (Chip74191Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_74191_binary_counter_process);
	munit_assert_ptr_equal(chip->destroy, chip_74191_binary_counter_destroy);

	// initialize with 'random' value
	chip->state = 0xfa;

	// load the counter
	SIGNAL_SET_BOOL(load_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(clk, false);
	SIGNAL_SET_BOOL(a, true);
	SIGNAL_SET_BOOL(b, false);
	SIGNAL_SET_BOOL(c, true);
	SIGNAL_SET_BOOL(d, true);
	signal_pool_cycle(chip->signal_pool, 1);
	chip_74191_binary_counter_process(chip);
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
		SIGNAL_SET_BOOL(load_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk, true);
		SIGNAL_SET_BOOL(d_u, truth_table[line][0]);
		SIGNAL_SET_BOOL(enable_b, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_74191_binary_counter_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(qa) == truth_table[line][2]);
		munit_assert(SIGNAL_NEXT_BOOL(qb) == truth_table[line][3]);
		munit_assert(SIGNAL_NEXT_BOOL(qc) == truth_table[line][4]);
		munit_assert(SIGNAL_NEXT_BOOL(qd) == truth_table[line][5]);
		munit_assert(SIGNAL_NEXT_BOOL(max_min) == truth_table[line][6]);
		munit_assert_true(SIGNAL_NEXT_BOOL(rco_b));

		// negative edge
		SIGNAL_SET_BOOL(load_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(clk, false);
		SIGNAL_SET_BOOL(d_u, truth_table[line][0]);
		SIGNAL_SET_BOOL(enable_b, truth_table[line][1]);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_74191_binary_counter_process(chip);
		munit_assert(SIGNAL_NEXT_BOOL(qa) == truth_table[line][2]);
		munit_assert(SIGNAL_NEXT_BOOL(qb) == truth_table[line][3]);
		munit_assert(SIGNAL_NEXT_BOOL(qc) == truth_table[line][4]);
		munit_assert(SIGNAL_NEXT_BOOL(qd) == truth_table[line][5]);
		munit_assert(SIGNAL_NEXT_BOOL(max_min) == truth_table[line][6]);
		munit_assert(SIGNAL_NEXT_BOOL(rco_b) == truth_table[line][7]);
	}

	simulator_destroy(chip->simulator);
	chip_74191_binary_counter_destroy(chip);
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

	Chip74244OctalBuffer *chip = chip_74244_octal_buffer_create(simulator_create(NS_TO_PS(100)), (Chip74244Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_74244_octal_buffer_process);
	munit_assert_ptr_equal(chip->destroy, chip_74244_octal_buffer_destroy);

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

			signal_pool_cycle(chip->signal_pool, 1);
			chip_74244_octal_buffer_process(chip);

			munit_assert(SIGNAL_NEXT_BOOL(y11) == (truth_table[i][0] ? false : BIT_IS_SET(input, 0)));
			munit_assert(SIGNAL_NEXT_BOOL(y12) == (truth_table[i][0] ? false : BIT_IS_SET(input, 1)));
			munit_assert(SIGNAL_NEXT_BOOL(y13) == (truth_table[i][0] ? false : BIT_IS_SET(input, 2)));
			munit_assert(SIGNAL_NEXT_BOOL(y14) == (truth_table[i][0] ? false : BIT_IS_SET(input, 3)));
			munit_assert(SIGNAL_NEXT_BOOL(y21) == (truth_table[i][1] ? false : BIT_IS_SET(input, 4)));
			munit_assert(SIGNAL_NEXT_BOOL(y22) == (truth_table[i][1] ? false : BIT_IS_SET(input, 5)));
			munit_assert(SIGNAL_NEXT_BOOL(y23) == (truth_table[i][1] ? false : BIT_IS_SET(input, 6)));
			munit_assert(SIGNAL_NEXT_BOOL(y24) == (truth_table[i][1] ? false : BIT_IS_SET(input, 7)));
		}
	}

	simulator_destroy(chip->simulator);
	chip_74244_octal_buffer_destroy(chip);

	return MUNIT_OK;
}

static MunitResult test_74373_latch(const MunitParameter params[], void *user_data_or_fixture) {

	Chip74373Latch *chip = chip_74373_latch_create(simulator_create(NS_TO_PS(100)), (Chip74373Signals) {0});
	munit_assert_ptr_not_null(chip);
	munit_assert_ptr_equal(chip->process, chip_74373_latch_process);
	munit_assert_ptr_equal(chip->destroy, chip_74373_latch_destroy);

	bool truth_table[][18] = {
		//  oc    c      d1     d2     d3     d4     d5     d6     d7     d8     q1     q2     q3     q4     q5     q6     q7     q8
		{false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true},
		{false, true,  true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  false},
		{false, false, false, false, false, false, false, false, false, false, true,  false, true,  false, true,  false, true,  false}
	};

	for (int line = 0; line < sizeof(truth_table) / sizeof(truth_table[0]); ++line) {
		munit_logf(MUNIT_LOG_INFO, "truth_table[%d]", line);

		SIGNAL_SET_BOOL(oc_b, truth_table[line][0]);
		SIGNAL_SET_BOOL(c, truth_table[line][1]);
		SIGNAL_SET_BOOL(d1, truth_table[line][2]);
		SIGNAL_SET_BOOL(d2, truth_table[line][3]);
		SIGNAL_SET_BOOL(d3, truth_table[line][4]);
		SIGNAL_SET_BOOL(d4, truth_table[line][5]);
		SIGNAL_SET_BOOL(d5, truth_table[line][6]);
		SIGNAL_SET_BOOL(d6, truth_table[line][7]);
		SIGNAL_SET_BOOL(d7, truth_table[line][8]);
		SIGNAL_SET_BOOL(d8, truth_table[line][9]);

		signal_pool_cycle(chip->signal_pool, 1);
		chip_74373_latch_process(chip);

		munit_assert(SIGNAL_NEXT_BOOL(q1) == truth_table[line][10]);
		munit_assert(SIGNAL_NEXT_BOOL(q2) == truth_table[line][11]);
		munit_assert(SIGNAL_NEXT_BOOL(q3) == truth_table[line][12]);
		munit_assert(SIGNAL_NEXT_BOOL(q4) == truth_table[line][13]);
		munit_assert(SIGNAL_NEXT_BOOL(q5) == truth_table[line][14]);
		munit_assert(SIGNAL_NEXT_BOOL(q6) == truth_table[line][15]);
		munit_assert(SIGNAL_NEXT_BOOL(q7) == truth_table[line][16]);
		munit_assert(SIGNAL_NEXT_BOOL(q8) == truth_table[line][17]);
	}

	simulator_destroy(chip->simulator);
	chip_74373_latch_destroy(chip);
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
