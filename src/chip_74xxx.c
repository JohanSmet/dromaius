// chip_74xxx.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of 74-family logic chips

#include "chip_74xxx.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_OWNER		chip

#define CHIP_74XXX_SET_FUNCTIONS(prefix)	\
		CHIP_SET_FUNCTIONS(chip, prefix##_process, prefix##_destroy)

#define CHIP_74XXX_SET_VARIABLES(pt)			\
		CHIP_SET_VARIABLES(chip, sim, chip->signals, (pt), SIGNAL_CONCAT(SIGNAL_PREFIX, PIN_COUNT))

///////////////////////////////////////////////////////////////////////////////
//
// 7400 - Quad 2 Input NAND gates
//

#define SIGNAL_PREFIX		CHIP_7400_

static uint8_t Chip7400_PinTypes[CHIP_7400_PIN_COUNT] = {
	[CHIP_7400_A1] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7400_A2] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7400_A3] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7400_A4] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7400_B1] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7400_B2] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7400_B3] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7400_B4] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7400_Y1] = CHIP_PIN_OUTPUT,
	[CHIP_7400_Y2] = CHIP_PIN_OUTPUT,
	[CHIP_7400_Y3] = CHIP_PIN_OUTPUT,
	[CHIP_7400_Y4] = CHIP_PIN_OUTPUT
};

static void chip_7400_nand_destroy(Chip7400Nand *chip);
static void chip_7400_nand_process(Chip7400Nand *chip);

Chip7400Nand *chip_7400_nand_create(Simulator *sim, Chip7400Signals signals) {
	Chip7400Nand *chip = (Chip7400Nand *) calloc(1, sizeof(Chip7400Nand));
	CHIP_74XXX_SET_FUNCTIONS(chip_7400_nand);
	CHIP_74XXX_SET_VARIABLES(Chip7400_PinTypes)

	chip->signal_pool = sim->signal_pool;
	memcpy(chip->signals, signals, sizeof(Chip7400Signals));
	SIGNAL_DEFINE(A1);
	SIGNAL_DEFINE(B1);
	SIGNAL_DEFINE(Y1);
	SIGNAL_DEFINE(A2);
	SIGNAL_DEFINE(B2);
	SIGNAL_DEFINE(Y2);
	SIGNAL_DEFINE(A3);
	SIGNAL_DEFINE(B3);
	SIGNAL_DEFINE(Y3);
	SIGNAL_DEFINE(A4);
	SIGNAL_DEFINE(B4);
	SIGNAL_DEFINE(Y4);

	return chip;
}

static void chip_7400_nand_destroy(Chip7400Nand *chip) {
	assert(chip);
	free(chip);
}

static void chip_7400_nand_process(Chip7400Nand *chip) {
	assert(chip);

	SIGNAL_WRITE(Y1, !(SIGNAL_READ(A1) && SIGNAL_READ(B1)));
	SIGNAL_WRITE(Y2, !(SIGNAL_READ(A2) && SIGNAL_READ(B2)));
	SIGNAL_WRITE(Y3, !(SIGNAL_READ(A3) && SIGNAL_READ(B3)));
	SIGNAL_WRITE(Y4, !(SIGNAL_READ(A4) && SIGNAL_READ(B4)));
}

///////////////////////////////////////////////////////////////////////////////
//
// 7474 - Dual Positive-Edge-Triggered D Flip-Flops with preset/clear
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_7474_

static uint8_t Chip7474_PinTypes[CHIP_7474_PIN_COUNT] = {
	[CHIP_7474_CLR1_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7474_PR1_B ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7474_CLK1  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7474_D1    ] = CHIP_PIN_INPUT,
	[CHIP_7474_Q1    ] = CHIP_PIN_OUTPUT,
	[CHIP_7474_Q1_B  ] = CHIP_PIN_OUTPUT,

	[CHIP_7474_CLR2_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7474_PR2_B ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7474_CLK2  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7474_D2    ] = CHIP_PIN_INPUT,
	[CHIP_7474_Q2_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_7474_Q2    ] = CHIP_PIN_OUTPUT
};

static void chip_7474_d_flipflop_destroy(Chip7474DFlipFlop *chip);
static void chip_7474_d_flipflop_process(Chip7474DFlipFlop *chip);

Chip7474DFlipFlop *chip_7474_d_flipflop_create(Simulator *sim, Chip7474Signals signals) {
	Chip7474DFlipFlop *chip = (Chip7474DFlipFlop *) calloc(1, sizeof(Chip7474DFlipFlop));
	CHIP_74XXX_SET_FUNCTIONS(chip_7474_d_flipflop);
	CHIP_74XXX_SET_VARIABLES(Chip7474_PinTypes)

	chip->signal_pool = sim->signal_pool;
	memcpy(chip->signals, signals, sizeof(Chip7474Signals));
	SIGNAL_DEFINE_DEFAULT(CLR1_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE(D1);
	SIGNAL_DEFINE(CLK1);
	SIGNAL_DEFINE_DEFAULT(PR1_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE(Q1);
	SIGNAL_DEFINE(Q1_B);
	SIGNAL_DEFINE(Q2_B);
	SIGNAL_DEFINE(Q2);
	SIGNAL_DEFINE_DEFAULT(PR2_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE(CLK2);
	SIGNAL_DEFINE(D2);
	SIGNAL_DEFINE_DEFAULT(CLR2_B, ACTLO_DEASSERT);

	SIGNAL_WRITE(Q1, chip->q1);
	SIGNAL_WRITE(Q1_B, chip->q1_b);
	SIGNAL_WRITE(Q2, chip->q2);
	SIGNAL_WRITE(Q2_B, chip->q2_b);

	return chip;
}

static void chip_7474_d_flipflop_destroy(Chip7474DFlipFlop *chip) {
	assert(chip);
	free(chip);
}

static void chip_7474_d_flipflop_process(Chip7474DFlipFlop *chip) {
	assert(chip);

	// flip-flop 1
	bool q1 = chip->q1;
	bool q1_b = chip->q1_b;

	if (ACTLO_ASSERTED(SIGNAL_READ(PR1_B)) && ACTLO_ASSERTED(SIGNAL_READ(CLR1_B))) {
		chip->q1 = true;
		chip->q1_b = true;
	} else if (ACTLO_ASSERTED(SIGNAL_READ(PR1_B))) {
		chip->q1 = true;
		chip->q1_b = false;
	} else if (ACTLO_ASSERTED(SIGNAL_READ(CLR1_B))) {
		chip->q1 = false;
		chip->q1_b = true;
	} else if (SIGNAL_READ(CLK1) && SIGNAL_CHANGED(CLK1)) {
		chip->q1 = SIGNAL_READ(D1);
		chip->q1_b = !chip->q1;
	}

	if (q1 != chip->q1) {
		SIGNAL_WRITE(Q1, chip->q1);
	}

	if (q1_b != chip->q1_b) {
		SIGNAL_WRITE(Q1_B, chip->q1_b);
	}

	// flip-flop 2
	bool q2 = chip->q2;
	bool q2_b = chip->q2_b;

	if (ACTLO_ASSERTED(SIGNAL_READ(PR2_B)) && ACTLO_ASSERTED(SIGNAL_READ(CLR2_B))) {
		chip->q2 = true;
		chip->q2_b = true;
	} else if (ACTLO_ASSERTED(SIGNAL_READ(PR2_B))) {
		chip->q2 = true;
		chip->q2_b = false;
	} else if (ACTLO_ASSERTED(SIGNAL_READ(CLR2_B))) {
		chip->q2 = false;
		chip->q2_b = true;
	} else if (SIGNAL_READ(CLK2) && SIGNAL_CHANGED(CLK2)) {
		chip->q2 = SIGNAL_READ(D2);
		chip->q2_b = !chip->q2;
	}

	if (q2 != chip->q2) {
		SIGNAL_WRITE(Q2, chip->q2);
	}

	if (q2_b != chip->q2_b) {
		SIGNAL_WRITE(Q2_B, chip->q2_b);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// 7493 - 4-Bit Binary Counter
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_7493_

static uint8_t Chip7493_PinTypes[CHIP_7493_PIN_COUNT] = {
	[CHIP_7493_A_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7493_B_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7493_R01] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7493_R02] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_7493_QA ] = CHIP_PIN_OUTPUT,
	[CHIP_7493_QB ] = CHIP_PIN_OUTPUT,
	[CHIP_7493_QC ] = CHIP_PIN_OUTPUT,
	[CHIP_7493_QD ] = CHIP_PIN_OUTPUT,
};

static void chip_7493_binary_counter_destroy(Chip7493BinaryCounter *chip);
static void chip_7493_binary_counter_process(Chip7493BinaryCounter *chip);

Chip7493BinaryCounter *chip_7493_binary_counter_create(Simulator *sim, Chip7493Signals signals) {
	Chip7493BinaryCounter *chip = (Chip7493BinaryCounter *) calloc(1, sizeof(Chip7493BinaryCounter));
	CHIP_74XXX_SET_FUNCTIONS(chip_7493_binary_counter);
	CHIP_74XXX_SET_VARIABLES(Chip7493_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip7493Signals));
	SIGNAL_DEFINE(B_B);
	SIGNAL_DEFINE_DEFAULT(R01, false);
	SIGNAL_DEFINE_DEFAULT(R02, false);
	SIGNAL_DEFINE(QC);
	SIGNAL_DEFINE(QB);
	SIGNAL_DEFINE(QD);
	SIGNAL_DEFINE(QA);
	SIGNAL_DEFINE(A_B);

	return chip;
}

static void chip_7493_binary_counter_destroy(Chip7493BinaryCounter *chip) {
	assert(chip);
	free(chip);
}

static void chip_7493_binary_counter_process(Chip7493BinaryCounter *chip) {
	assert(chip);

	if (ACTHI_ASSERTED(SIGNAL_READ(R01)) && ACTHI_ASSERTED(SIGNAL_READ(R02))) {
		chip->count_a = 0;
		chip->count_b = 0;
	} else {
		bool trigger_a = SIGNAL_CHANGED(A_B) && !SIGNAL_READ(A_B);

		// xor is basically a conditional negation
		chip->count_a = chip->count_a ^ trigger_a;

		// ok, a bit hacky ... if the second stage's clock is connected to the output of the first stage:
		// don't wait until the next process call to trigger the second stage
		if (signal_equal(SIGNAL(B_B), SIGNAL(QA))) {
			chip->count_b += trigger_a & !chip->count_a;
		} else {
			bool trigger_b = SIGNAL_CHANGED(B_B) && !SIGNAL_READ(B_B);
			chip->count_b += trigger_b;
		}
	}

	SIGNAL_WRITE(QA, chip->count_a);
	SIGNAL_WRITE(QB, chip->count_b & 0b0001);
	SIGNAL_WRITE(QC, (chip->count_b & 0b0010) >> 1);
	SIGNAL_WRITE(QD, (chip->count_b & 0b0100) >> 2);
}

///////////////////////////////////////////////////////////////////////////////
//
// 74107 - Dual J-K Flip-Flops with clear
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74107_

static uint8_t Chip74107_PinTypes[CHIP_74107_PIN_COUNT] = {
	[CHIP_74107_CLK1  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74107_CLR1_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74107_J1    ] = CHIP_PIN_INPUT,
	[CHIP_74107_K1    ] = CHIP_PIN_INPUT,
	[CHIP_74107_Q1_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74107_Q1    ] = CHIP_PIN_OUTPUT,

	[CHIP_74107_CLK2  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74107_CLR2_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74107_J2    ] = CHIP_PIN_INPUT,
	[CHIP_74107_K2    ] = CHIP_PIN_INPUT,
	[CHIP_74107_Q2    ] = CHIP_PIN_OUTPUT,
	[CHIP_74107_Q2_B  ] = CHIP_PIN_OUTPUT,
};

static void chip_74107_jk_flipflop_destroy(Chip74107JKFlipFlop *chip);
static void chip_74107_jk_flipflop_process(Chip74107JKFlipFlop *chip);

Chip74107JKFlipFlop *chip_74107_jk_flipflop_create(Simulator *sim, Chip74107Signals signals) {
	Chip74107JKFlipFlop *chip = (Chip74107JKFlipFlop *) calloc(1, sizeof(Chip74107JKFlipFlop));
	CHIP_74XXX_SET_FUNCTIONS(chip_74107_jk_flipflop);
	CHIP_74XXX_SET_VARIABLES(Chip74107_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74107Signals));
	SIGNAL_DEFINE(J1);
	SIGNAL_DEFINE(Q1_B);
	SIGNAL_DEFINE(Q1);
	SIGNAL_DEFINE(K1);
	SIGNAL_DEFINE(Q2);
	SIGNAL_DEFINE(Q2_B);
	SIGNAL_DEFINE(J2);
	SIGNAL_DEFINE(CLK2);
	SIGNAL_DEFINE_DEFAULT(CLR2_B, true);
	SIGNAL_DEFINE(K2);
	SIGNAL_DEFINE(CLK1);
	SIGNAL_DEFINE_DEFAULT(CLR1_B, true);

	SIGNAL_WRITE(Q1, chip->q1);
	SIGNAL_WRITE(Q1_B, !chip->q1);
	SIGNAL_WRITE(Q2, chip->q2);
	SIGNAL_WRITE(Q2_B, !chip->q2);

	return chip;
}

static void chip_74107_jk_flipflop_destroy(Chip74107JKFlipFlop *chip) {
	assert(chip);
	free(chip);
}

static void chip_74107_jk_flipflop_process(Chip74107JKFlipFlop *chip) {
	assert(chip);

	// flip-flop 1
	bool q1 = chip->q1;
	if (ACTLO_ASSERTED(SIGNAL_READ(CLR1_B))) {
		chip->q1 = false;
	} else if (!SIGNAL_READ(CLK1) && SIGNAL_CHANGED(CLK1)) {
		if (SIGNAL_READ(J1) && SIGNAL_READ(K1)) {
			chip->q1 = !chip->q1;
		} else if (SIGNAL_READ(J1) || SIGNAL_READ(K1)) {
			chip->q1 = SIGNAL_READ(J1);
		}
	}

	if (q1 != chip->q1) {
		SIGNAL_WRITE(Q1, chip->q1);
		SIGNAL_WRITE(Q1_B, !chip->q1);
	}

	// flip-flop 2
	bool q2 = chip->q2;
	if (ACTLO_ASSERTED(SIGNAL_READ(CLR2_B))) {
		chip->q2 = false;
	} else if (!SIGNAL_READ(CLK2) && SIGNAL_CHANGED(CLK2)) {
		if (SIGNAL_READ(J2) && SIGNAL_READ(K2)) {
			chip->q2 = !chip->q2;
		} else if (SIGNAL_READ(J2) || SIGNAL_READ(K2)) {
			chip->q2 = SIGNAL_READ(J2);
		}
	}

	if (q2 != chip->q2) {
		SIGNAL_WRITE(Q2, chip->q2);
		SIGNAL_WRITE(Q2_B, !chip->q2);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// 74145 - BCD-to-Decimal Decoder/driver
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74145_

static uint8_t Chip74145_PinTypes[CHIP_74145_PIN_COUNT] = {
	[CHIP_74145_A    ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74145_B    ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74145_C    ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74145_D    ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74145_Y0_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74145_Y1_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74145_Y2_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74145_Y3_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74145_Y4_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74145_Y5_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74145_Y6_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74145_Y7_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74145_Y8_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74145_Y9_B ] = CHIP_PIN_OUTPUT,
};

static void chip_74145_bcd_decoder_destroy(Chip74145BcdDecoder *chip);
static void chip_74145_bcd_decoder_process(Chip74145BcdDecoder *chip);

typedef struct Chip74145BcdDecoder_private {
	Chip74145BcdDecoder	intf;
	Signal *			outputs[10];
} Chip74145BcdDecoder_private;

Chip74145BcdDecoder *chip_74145_bcd_decoder_create(Simulator *sim, Chip74145Signals signals) {
	Chip74145BcdDecoder_private *priv = (Chip74145BcdDecoder_private *) calloc(1, sizeof(Chip74145BcdDecoder_private));
	Chip74145BcdDecoder *chip = &priv->intf;
	CHIP_74XXX_SET_FUNCTIONS(chip_74145_bcd_decoder);
	CHIP_74XXX_SET_VARIABLES(Chip74145_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74145Signals));
	SIGNAL_DEFINE(Y0_B);
	SIGNAL_DEFINE(Y1_B);
	SIGNAL_DEFINE(Y2_B);
	SIGNAL_DEFINE(Y3_B);
	SIGNAL_DEFINE(Y4_B);
	SIGNAL_DEFINE(Y5_B);
	SIGNAL_DEFINE(Y6_B);
	SIGNAL_DEFINE(Y7_B);
	SIGNAL_DEFINE(Y8_B);
	SIGNAL_DEFINE(Y9_B);
	SIGNAL_DEFINE(D);
	SIGNAL_DEFINE(C);
	SIGNAL_DEFINE(B);
	SIGNAL_DEFINE(A);

	priv->outputs[0]  = &SIGNAL(Y0_B);	priv->outputs[1]  = &SIGNAL(Y1_B);
	priv->outputs[2]  = &SIGNAL(Y2_B);	priv->outputs[3]  = &SIGNAL(Y3_B);
	priv->outputs[4]  = &SIGNAL(Y4_B);	priv->outputs[5]  = &SIGNAL(Y5_B);
	priv->outputs[6]  = &SIGNAL(Y6_B);	priv->outputs[7]  = &SIGNAL(Y7_B);
	priv->outputs[8]  = &SIGNAL(Y8_B);	priv->outputs[9]  = &SIGNAL(Y9_B);

	return chip;
}

static void chip_74145_bcd_decoder_destroy(Chip74145BcdDecoder *chip) {
	assert((Chip74145BcdDecoder_private *) chip);
	free(chip);
}

static void chip_74145_bcd_decoder_process(Chip74145BcdDecoder *chip) {
	assert(chip);

	Signal **outputs = ((Chip74145BcdDecoder_private *) chip)->outputs;

	for (int i = 0; i < 10; ++i) {
		signal_write(chip->signal_pool, *outputs[i], ACTLO_DEASSERT);
	}

	int value = SIGNAL_READ(A) |
				SIGNAL_READ(B) << 1 |
				SIGNAL_READ(C) << 2 |
				SIGNAL_READ(D) << 3;

	if (value < 10) {
		signal_write(chip->signal_pool, *outputs[value], ACTLO_ASSERT);
	}

}

///////////////////////////////////////////////////////////////////////////////
//
// 74153 - Dual 4-Line to 1-Line Data Selectors/Multiplexers
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74153_

static uint8_t Chip74153_PinTypes[CHIP_74153_PIN_COUNT] = {
	[CHIP_74153_G1 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_G2 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_A  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_B  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_C10] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_C11] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_C12] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_C13] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_C20] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_C21] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_C22] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_C23] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74153_Y1 ] = CHIP_PIN_OUTPUT,
	[CHIP_74153_Y2 ] = CHIP_PIN_OUTPUT,
};

static void chip_74153_multiplexer_destroy(Chip74153Multiplexer *chip);
static void chip_74153_multiplexer_process(Chip74153Multiplexer *chip);

Chip74153Multiplexer *chip_74153_multiplexer_create(Simulator *sim, Chip74153Signals signals) {

	Chip74153Multiplexer *chip = (Chip74153Multiplexer *) calloc(1, sizeof(Chip74153Multiplexer));
	CHIP_74XXX_SET_FUNCTIONS(chip_74153_multiplexer);
	CHIP_74XXX_SET_VARIABLES(Chip74153_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74153Signals));
	SIGNAL_DEFINE(G1);
	SIGNAL_DEFINE(B);
	SIGNAL_DEFINE(C13);
	SIGNAL_DEFINE(C12);
	SIGNAL_DEFINE(C11);
	SIGNAL_DEFINE(C10);
	SIGNAL_DEFINE(Y1);
	SIGNAL_DEFINE(Y2);
	SIGNAL_DEFINE(C20);
	SIGNAL_DEFINE(C21);
	SIGNAL_DEFINE(C22);
	SIGNAL_DEFINE(C23);
	SIGNAL_DEFINE(A);
	SIGNAL_DEFINE(G2);

	chip->inputs[0][0] = &SIGNAL(C10);	chip->inputs[1][0] = &SIGNAL(C20);
	chip->inputs[0][1] = &SIGNAL(C11);	chip->inputs[1][1] = &SIGNAL(C21);
	chip->inputs[0][2] = &SIGNAL(C12);	chip->inputs[1][2] = &SIGNAL(C22);
	chip->inputs[0][3] = &SIGNAL(C13);	chip->inputs[1][3] = &SIGNAL(C23);

	return chip;
}

static void chip_74153_multiplexer_destroy(Chip74153Multiplexer *chip) {
	assert(chip);
	free(chip);
}

static void chip_74153_multiplexer_process(Chip74153Multiplexer *chip) {
	assert(chip);

	int index = SIGNAL_READ(A) | SIGNAL_READ(B) << 1;
	SIGNAL_WRITE(Y1, !SIGNAL_READ(G1) && signal_read(chip->signal_pool, *chip->inputs[0][index]));
	SIGNAL_WRITE(Y2, !SIGNAL_READ(G2) && signal_read(chip->signal_pool, *chip->inputs[1][index]));
}

///////////////////////////////////////////////////////////////////////////////
//
// 74154 - 4-Line to 16-Line Decoder/Multiplexer
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74154_

static uint8_t Chip74154_PinTypes[CHIP_74154_PIN_COUNT] = {
	[CHIP_74154_G1_B  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74154_G2_B  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74154_A     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74154_B     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74154_C     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74154_D     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74154_Y0_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y1_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y2_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y3_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y4_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y5_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y6_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y7_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y8_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y9_B  ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y10_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y11_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y12_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y13_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y14_B ] = CHIP_PIN_OUTPUT,
	[CHIP_74154_Y15_B ] = CHIP_PIN_OUTPUT,
};

static void chip_74154_decoder_destroy(Chip74154Decoder *chip);
static void chip_74154_decoder_process(Chip74154Decoder *chip);

typedef struct Chip74154Decoder_private {
	Chip74154Decoder	intf;
	Signal *			outputs[16];
} Chip74154Decoder_private;

Chip74154Decoder *chip_74154_decoder_create(Simulator *sim, Chip74154Signals signals) {
	Chip74154Decoder_private *priv = (Chip74154Decoder_private *) calloc(1, sizeof(Chip74154Decoder_private));
	Chip74154Decoder *chip = &priv->intf;
	CHIP_74XXX_SET_FUNCTIONS(chip_74154_decoder);
	CHIP_74XXX_SET_VARIABLES(Chip74154_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74154Signals));
	SIGNAL_DEFINE(Y0_B);
	SIGNAL_DEFINE(Y1_B);
	SIGNAL_DEFINE(Y2_B);
	SIGNAL_DEFINE(Y3_B);
	SIGNAL_DEFINE(Y4_B);
	SIGNAL_DEFINE(Y5_B);
	SIGNAL_DEFINE(Y6_B);
	SIGNAL_DEFINE(Y7_B);
	SIGNAL_DEFINE(Y8_B);
	SIGNAL_DEFINE(Y9_B);
	SIGNAL_DEFINE(Y10_B);
	SIGNAL_DEFINE(Y11_B);
	SIGNAL_DEFINE(Y12_B);
	SIGNAL_DEFINE(Y13_B);
	SIGNAL_DEFINE(Y14_B);
	SIGNAL_DEFINE(Y15_B);
	SIGNAL_DEFINE(G1_B);
	SIGNAL_DEFINE(G2_B);
	SIGNAL_DEFINE(D);
	SIGNAL_DEFINE(C);
	SIGNAL_DEFINE(B);
	SIGNAL_DEFINE(A);

	priv->outputs[0]  = &SIGNAL(Y0_B);	priv->outputs[1]  = &SIGNAL(Y1_B);
	priv->outputs[2]  = &SIGNAL(Y2_B);	priv->outputs[3]  = &SIGNAL(Y3_B);
	priv->outputs[4]  = &SIGNAL(Y4_B);	priv->outputs[5]  = &SIGNAL(Y5_B);
	priv->outputs[6]  = &SIGNAL(Y6_B);	priv->outputs[7]  = &SIGNAL(Y7_B);
	priv->outputs[8]  = &SIGNAL(Y8_B);	priv->outputs[9]  = &SIGNAL(Y9_B);
	priv->outputs[10] = &SIGNAL(Y10_B); priv->outputs[11] = &SIGNAL(Y11_B);
	priv->outputs[12] = &SIGNAL(Y12_B); priv->outputs[13] = &SIGNAL(Y13_B);
	priv->outputs[14] = &SIGNAL(Y14_B); priv->outputs[15] = &SIGNAL(Y15_B);

	return chip;
}

static void chip_74154_decoder_destroy(Chip74154Decoder *chip) {
	assert((Chip74154Decoder_private *) chip);
	free(chip);
}

static void chip_74154_decoder_process(Chip74154Decoder *chip) {
	assert(chip);

	Signal **outputs = ((Chip74154Decoder_private *) chip)->outputs;

	for (int i = 0; i < 16; ++i) {
		signal_write(chip->signal_pool, *outputs[i], ACTLO_DEASSERT);
	}

	if (!(ACTLO_ASSERTED(SIGNAL_READ(G1_B)) && ACTLO_ASSERTED(SIGNAL_READ(G2_B)))) {
		return;
	}

	int value = SIGNAL_READ(A) |
				SIGNAL_READ(B) << 1 |
				SIGNAL_READ(C) << 2 |
				SIGNAL_READ(D) << 3;
	signal_write(chip->signal_pool, *outputs[value], ACTLO_ASSERT);
}

///////////////////////////////////////////////////////////////////////////////
//
// 74157 - Quad 2-Input Multiplexer
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74157_

static uint8_t Chip74157_PinTypes[CHIP_74157_PIN_COUNT] = {
	[CHIP_74157_SEL]      = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74157_ENABLE_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74157_I0A     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74157_I0B     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74157_I0C     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74157_I0D     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74157_I1A     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74157_I1B     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74157_I1C     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74157_I1D     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74157_ZA      ] = CHIP_PIN_OUTPUT,
	[CHIP_74157_ZB      ] = CHIP_PIN_OUTPUT,
	[CHIP_74157_ZC      ] = CHIP_PIN_OUTPUT,
	[CHIP_74157_ZD      ] = CHIP_PIN_OUTPUT,
};

static void chip_74157_multiplexer_destroy(Chip74157Multiplexer *chip);
static void chip_74157_multiplexer_process(Chip74157Multiplexer *chip);

Chip74157Multiplexer *chip_74157_multiplexer_create(Simulator *sim, Chip74157Signals signals) {
	Chip74157Multiplexer *chip = (Chip74157Multiplexer *) calloc(1, sizeof(Chip74157Multiplexer));
	CHIP_74XXX_SET_FUNCTIONS(chip_74157_multiplexer);
	CHIP_74XXX_SET_VARIABLES(Chip74157_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74157Signals));
	SIGNAL_DEFINE(SEL);
	SIGNAL_DEFINE(I0A);
	SIGNAL_DEFINE(I1A);
	SIGNAL_DEFINE(ZA);
	SIGNAL_DEFINE(I0B);
	SIGNAL_DEFINE(I1B);
	SIGNAL_DEFINE(ZB);
	SIGNAL_DEFINE(ZD);
	SIGNAL_DEFINE(I1D);
	SIGNAL_DEFINE(I0D);
	SIGNAL_DEFINE(ZC);
	SIGNAL_DEFINE(I1C);
	SIGNAL_DEFINE(I0C);
	SIGNAL_DEFINE(ENABLE_B);

	return chip;
}

static void chip_74157_multiplexer_destroy(Chip74157Multiplexer *chip) {
	assert(chip);
	free(chip);
}

static void chip_74157_multiplexer_process(Chip74157Multiplexer *chip) {
	assert(chip);

	bool mask_0 = !SIGNAL_READ(ENABLE_B) && !SIGNAL_READ(SEL);
	bool mask_1 = !SIGNAL_READ(ENABLE_B) && SIGNAL_READ(SEL);

	SIGNAL_WRITE(ZA, (SIGNAL_READ(I0A) & mask_0) || (SIGNAL_READ(I1A) & mask_1));
	SIGNAL_WRITE(ZB, (SIGNAL_READ(I0B) & mask_0) || (SIGNAL_READ(I1B) & mask_1));
	SIGNAL_WRITE(ZC, (SIGNAL_READ(I0C) & mask_0) || (SIGNAL_READ(I1C) & mask_1));
	SIGNAL_WRITE(ZD, (SIGNAL_READ(I0D) & mask_0) || (SIGNAL_READ(I1D) & mask_1));
}

///////////////////////////////////////////////////////////////////////////////
//
// 74164 - 8-Bit Serial In/Parallel Out Shift Register
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74164_

static uint8_t Chip74164_PinTypes[CHIP_74164_PIN_COUNT] = {
	[CHIP_74164_CLK    ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74164_CLEAR_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74164_A      ] = CHIP_PIN_INPUT,
	[CHIP_74164_B      ] = CHIP_PIN_INPUT,
	[CHIP_74164_QA     ] = CHIP_PIN_OUTPUT,
	[CHIP_74164_QB     ] = CHIP_PIN_OUTPUT,
	[CHIP_74164_QC     ] = CHIP_PIN_OUTPUT,
	[CHIP_74164_QD     ] = CHIP_PIN_OUTPUT,
	[CHIP_74164_QE     ] = CHIP_PIN_OUTPUT,
	[CHIP_74164_QF     ] = CHIP_PIN_OUTPUT,
	[CHIP_74164_QG     ] = CHIP_PIN_OUTPUT,
	[CHIP_74164_QH     ] = CHIP_PIN_OUTPUT,
};

static void chip_74164_shift_register_destroy(Chip74164ShiftRegister *chip);
static void chip_74164_shift_register_process(Chip74164ShiftRegister *chip);

Chip74164ShiftRegister *chip_74164_shift_register_create(Simulator *sim, Chip74164Signals signals) {
	Chip74164ShiftRegister *chip = (Chip74164ShiftRegister *) calloc(1, sizeof(Chip74164ShiftRegister));
	CHIP_74XXX_SET_FUNCTIONS(chip_74164_shift_register);
	CHIP_74XXX_SET_VARIABLES(Chip74164_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74164Signals));
	SIGNAL_DEFINE(A);
	SIGNAL_DEFINE(B);
	SIGNAL_DEFINE(QA);
	SIGNAL_DEFINE(QB);
	SIGNAL_DEFINE(QC);
	SIGNAL_DEFINE(QD);
	SIGNAL_DEFINE(CLK);
	SIGNAL_DEFINE_DEFAULT(CLEAR_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE(QE);
	SIGNAL_DEFINE(QF);
	SIGNAL_DEFINE(QG);
	SIGNAL_DEFINE(QH);

	return chip;
}

static void chip_74164_shift_register_destroy(Chip74164ShiftRegister *chip) {
	assert(chip);
	free(chip);
}

static void chip_74164_shift_register_process(Chip74164ShiftRegister *chip) {
	assert(chip);


	uint8_t state = chip->state;

	if (ACTLO_ASSERTED(SIGNAL_READ(CLEAR_B))) {
		chip->state = 0;
	} else if (SIGNAL_READ(CLK) && SIGNAL_CHANGED(CLK)) {
		// shift on the positive clock edge
		bool in = SIGNAL_READ(A) && SIGNAL_READ(B);
		chip->state = (uint8_t) (((chip->state << 1) | in) & 0xff);
	} else {
		// state not changed, nothing to output
		return;
	}

	if (state == chip->state) {
		return;
	}

	if ((chip->state & 0b00000001) != (state & 0b00000001)) {
		SIGNAL_WRITE(QA, chip->state & 0x1);
	}
	if ((chip->state & 0b00000010) != (state & 0b00000010)) {
		SIGNAL_WRITE(QB, (chip->state >> 1) & 0x1);
	}
	if ((chip->state & 0b00000100) != (state & 0b00000100)) {
		SIGNAL_WRITE(QC, (chip->state >> 2) & 0x1);
	}
	if ((chip->state & 0b00001000) != (state & 0b00001000)) {
		SIGNAL_WRITE(QD, (chip->state >> 3) & 0x1);
	}
	if ((chip->state & 0b00010000) != (state & 0b00010000)) {
		SIGNAL_WRITE(QE, (chip->state >> 4) & 0x1);
	}
	if ((chip->state & 0b00100000) != (state & 0b00100000)) {
		SIGNAL_WRITE(QF, (chip->state >> 5) & 0x1);
	}
	if ((chip->state & 0b01000000) != (state & 0b01000000)) {
		SIGNAL_WRITE(QG, (chip->state >> 6) & 0x1);
	}
	if ((chip->state & 0b10000000) != (state & 0b10000000)) {
		SIGNAL_WRITE(QH, (chip->state >> 7) & 0x1);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// 74165 - 8-Bit Parallel In/Serial Out Shift Registers
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74165_

static uint8_t Chip74165_PinTypes[CHIP_74165_PIN_COUNT] = {
	[CHIP_74165_SL      ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74165_CLK     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74165_SI      ] = CHIP_PIN_INPUT,
	[CHIP_74165_CLK_INH ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74165_A       ] = CHIP_PIN_INPUT,
	[CHIP_74165_B       ] = CHIP_PIN_INPUT,
	[CHIP_74165_C       ] = CHIP_PIN_INPUT,
	[CHIP_74165_D       ] = CHIP_PIN_INPUT,
	[CHIP_74165_E       ] = CHIP_PIN_INPUT,
	[CHIP_74165_F       ] = CHIP_PIN_INPUT,
	[CHIP_74165_G       ] = CHIP_PIN_INPUT,
	[CHIP_74165_H       ] = CHIP_PIN_INPUT,
	[CHIP_74165_QH      ] = CHIP_PIN_OUTPUT,
	[CHIP_74165_QH_B    ] = CHIP_PIN_OUTPUT,
};

static void chip_74165_shift_register_destroy(Chip74165ShiftRegister *chip);
static void chip_74165_shift_register_process(Chip74165ShiftRegister *chip);

Chip74165ShiftRegister *chip_74165_shift_register_create(Simulator *sim, Chip74165Signals signals) {

	Chip74165ShiftRegister *chip = (Chip74165ShiftRegister *) calloc(1, sizeof(Chip74165ShiftRegister));
	CHIP_74XXX_SET_FUNCTIONS(chip_74165_shift_register);
	CHIP_74XXX_SET_VARIABLES(Chip74165_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74165Signals));
	SIGNAL_DEFINE(SL);
	SIGNAL_DEFINE(CLK);
	SIGNAL_DEFINE(E);
	SIGNAL_DEFINE(F);
	SIGNAL_DEFINE(G);
	SIGNAL_DEFINE(H);
	SIGNAL_DEFINE(QH_B);
	SIGNAL_DEFINE(QH);
	SIGNAL_DEFINE(SI);
	SIGNAL_DEFINE(A);
	SIGNAL_DEFINE(B);
	SIGNAL_DEFINE(C);
	SIGNAL_DEFINE(D);
	SIGNAL_DEFINE(CLK_INH);

	return chip;
}

static void chip_74165_shift_register_destroy(Chip74165ShiftRegister *chip) {
	assert(chip);
	free(chip);
}

static void chip_74165_shift_register_process(Chip74165ShiftRegister *chip) {
	assert(chip);

	// data at the parallel inputs are loaded directly into the register on a HIGH-to-LOW transition of
	// the shit/load input, regardless of the logic levels on the clock, clock inhibit, or serial inputs.
	if (!SIGNAL_READ(SL) && SIGNAL_CHANGED(SL)) {
		bool value_h = SIGNAL_READ(H);

		chip->state = SIGNAL_READ(A);
		chip->state = (chip->state << 1) | SIGNAL_READ(B);
		chip->state = (chip->state << 1) | SIGNAL_READ(C);
		chip->state = (chip->state << 1) | SIGNAL_READ(D);
		chip->state = (chip->state << 1) | SIGNAL_READ(E);
		chip->state = (chip->state << 1) | SIGNAL_READ(F);
		chip->state = (chip->state << 1) | SIGNAL_READ(G);
		chip->state = (chip->state << 1) | value_h;

		SIGNAL_WRITE(QH, value_h);
		SIGNAL_WRITE(QH_B, !value_h);
		return;
	}

	bool gated_clk = !(SIGNAL_READ(CLK) || SIGNAL_READ(CLK_INH));

	if (!gated_clk && chip->prev_gated_clk) {
		chip->state = ((int) SIGNAL_READ(SI) << 7) | (chip->state >> 1);
	}

	bool output = chip->state & 0x01;
	SIGNAL_WRITE(QH, output);
	SIGNAL_WRITE(QH_B, !output);
	chip->prev_gated_clk = gated_clk;
}


///////////////////////////////////////////////////////////////////////////////
//
// 74177 - Presettable Binary Counter/Latch
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74177_

static uint8_t Chip74177_PinTypes[CHIP_74177_PIN_COUNT] = {
	[CHIP_74177_LOAD_B   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74177_CLEAR_B  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74177_CLK1     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74177_CLK2     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74177_A        ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74177_B        ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74177_C        ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74177_D        ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74177_QA       ] = CHIP_PIN_OUTPUT,
	[CHIP_74177_QB       ] = CHIP_PIN_OUTPUT,
	[CHIP_74177_QC       ] = CHIP_PIN_OUTPUT,
	[CHIP_74177_QD       ] = CHIP_PIN_OUTPUT,
};

static void chip_74177_binary_counter_destroy(Chip74177BinaryCounter *chip);
static void chip_74177_binary_counter_process(Chip74177BinaryCounter *chip);

Chip74177BinaryCounter *chip_74177_binary_counter_create(Simulator *sim, Chip74177Signals signals) {
	Chip74177BinaryCounter *chip = (Chip74177BinaryCounter *) calloc(1, sizeof(Chip74177BinaryCounter));
	CHIP_74XXX_SET_FUNCTIONS(chip_74177_binary_counter);
	CHIP_74XXX_SET_VARIABLES(Chip74177_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74177Signals));
	SIGNAL_DEFINE_DEFAULT(LOAD_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE(QC);
	SIGNAL_DEFINE(C);
	SIGNAL_DEFINE(A);
	SIGNAL_DEFINE(QA);
	SIGNAL_DEFINE(CLK2);
	SIGNAL_DEFINE(CLK1);
	SIGNAL_DEFINE(QB);
	SIGNAL_DEFINE(B);
	SIGNAL_DEFINE(D);
	SIGNAL_DEFINE(QD);
	SIGNAL_DEFINE_DEFAULT(CLEAR_B, ACTLO_DEASSERT);

	return chip;
}

static void chip_74177_binary_counter_destroy(Chip74177BinaryCounter *chip) {
	assert(chip);
	free(chip);
}

static void chip_74177_binary_counter_process(Chip74177BinaryCounter *chip) {
	assert(chip);

	if (ACTLO_ASSERTED(SIGNAL_READ(CLEAR_B))) {
		chip->count_1 = false;
		chip->count_2 = false;
	} else if (ACTLO_ASSERTED(SIGNAL_READ(LOAD_B))) {
		chip->count_1 = SIGNAL_READ(A);
		chip->count_2 = SIGNAL_READ(B) | (SIGNAL_READ(C) << 1) | (SIGNAL_READ(D) << 2);
	} else {
		bool trigger_1 = !SIGNAL_READ(CLK1) && SIGNAL_CHANGED(CLK1);
		chip->count_1 = chip->count_1 ^ trigger_1;

		// ok, a bit hacky ... if the second stage's clock is connected to the output of the first stage:
		// don't wait until the next process call to trigger the second stage
		if (signal_equal(SIGNAL(CLK2), SIGNAL(QA))) {
			chip->count_2 += trigger_1 & !chip->count_1;
		} else {
			bool trigger_2 = !SIGNAL_READ(CLK2) && SIGNAL_CHANGED(CLK2);
			chip->count_2 += trigger_2;
		}
	}

	SIGNAL_WRITE(QA, chip->count_1);
	SIGNAL_WRITE(QB, (chip->count_2 >> 0) & 0x01);
	SIGNAL_WRITE(QC, (chip->count_2 >> 1) & 0x01);
	SIGNAL_WRITE(QD, (chip->count_2 >> 2) & 0x01);
}

///////////////////////////////////////////////////////////////////////////////
//
// 74191 - 4-Bit Synchronous Up/Down Binary Counter
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74191_

static uint8_t Chip74191_PinTypes[CHIP_74191_PIN_COUNT] = {
	[CHIP_74191_ENABLE_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74191_LOAD_B  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74191_CLK     ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74191_A       ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74191_B       ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74191_C       ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74191_D       ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74191_D_U     ] = CHIP_PIN_INPUT,
	[CHIP_74191_QA      ] = CHIP_PIN_OUTPUT,
	[CHIP_74191_QB      ] = CHIP_PIN_OUTPUT,
	[CHIP_74191_QC      ] = CHIP_PIN_OUTPUT,
	[CHIP_74191_QD      ] = CHIP_PIN_OUTPUT,
	[CHIP_74191_MAX_MIN ] = CHIP_PIN_OUTPUT,
	[CHIP_74191_RCO_B   ] = CHIP_PIN_OUTPUT,
};

static void chip_74191_binary_counter_destroy(Chip74191BinaryCounter *chip);
static void chip_74191_binary_counter_process(Chip74191BinaryCounter *chip);

Chip74191BinaryCounter *chip_74191_binary_counter_create(Simulator *sim, Chip74191Signals signals) {

	Chip74191BinaryCounter *chip = (Chip74191BinaryCounter *) calloc(1, sizeof(Chip74191BinaryCounter));
	CHIP_74XXX_SET_FUNCTIONS(chip_74191_binary_counter);
	CHIP_74XXX_SET_VARIABLES(Chip74191_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74191Signals));
	SIGNAL_DEFINE(B);
	SIGNAL_DEFINE(QB);
	SIGNAL_DEFINE(QA);
	SIGNAL_DEFINE_DEFAULT(ENABLE_B, ACTLO_ASSERT);
	SIGNAL_DEFINE_DEFAULT(D_U, ACTHI_DEASSERT);
	SIGNAL_DEFINE(QC);
	SIGNAL_DEFINE(QD);
	SIGNAL_DEFINE(D);
	SIGNAL_DEFINE(C);
	SIGNAL_DEFINE_DEFAULT(LOAD_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE(MAX_MIN);
	SIGNAL_DEFINE(RCO_B);
	SIGNAL_DEFINE(CLK);
	SIGNAL_DEFINE(A);

	return chip;
}

static void chip_74191_binary_counter_destroy(Chip74191BinaryCounter *chip) {
	assert(chip);
	free(chip);
}

static void chip_74191_binary_counter_process(Chip74191BinaryCounter *chip) {
	assert(chip);

	bool clk = SIGNAL_READ(CLK);
	bool clk_edge = SIGNAL_CHANGED(CLK);
	bool d_u = SIGNAL_READ(D_U);
	bool rco = true;

	if (ACTLO_ASSERTED(SIGNAL_READ(LOAD_B))) {
		chip->state = (SIGNAL_READ(A) << 0) |
					  (SIGNAL_READ(B) << 1) |
					  (SIGNAL_READ(C) << 2) |
					  (SIGNAL_READ(D) << 3);

		SIGNAL_WRITE(QA, (chip->state & 0x1));
		SIGNAL_WRITE(QB, (chip->state >> 1) & 0x1);
		SIGNAL_WRITE(QC, (chip->state >> 2) & 0x1);
		SIGNAL_WRITE(QD, (chip->state >> 3) & 0x1);
	} else if (ACTLO_ASSERTED(SIGNAL_READ(ENABLE_B)) && clk && clk_edge) {
		// count on the positive clock edge
		int state = chip->state;
		chip->state = (chip->state - d_u + !d_u) & 0xf;
		chip->max_min = (d_u && chip->state == 0) || (!d_u && chip->state == 0xf);

		if ((state & 0b0001) != (chip->state & 0b0001)) {
			SIGNAL_WRITE(QA, (chip->state & 0x1));
		}
		if ((state & 0b0010) != (chip->state & 0b0010)) {
			SIGNAL_WRITE(QB, (chip->state >> 1) & 0x1);
		}
		if ((state & 0b0100) != (chip->state & 0b0100)) {
			SIGNAL_WRITE(QC, (chip->state >> 2) & 0x1);
		}
		if ((state & 0b1000) != (chip->state & 0b1000)) {
			SIGNAL_WRITE(QD, (chip->state >> 3) & 0x1);
		}
		SIGNAL_WRITE(MAX_MIN, chip->max_min);
	} else if (!clk && clk_edge) {
		// negative clock edge
		rco = !chip->max_min;
	}
	SIGNAL_WRITE(RCO_B, rco);
}

///////////////////////////////////////////////////////////////////////////////
//
// 74244 - Octal 3-STATE Buffer/Line Driver/Line Receiver
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74244_

static uint8_t Chip74244_PinTypes[CHIP_74244_PIN_COUNT] = {
	[CHIP_74244_G1_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74244_G2_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74244_A11 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74244_A12 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74244_A13 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74244_A14 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74244_A21 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74244_A22 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74244_A23 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74244_A24 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74244_Y11 ] = CHIP_PIN_OUTPUT,
	[CHIP_74244_Y12 ] = CHIP_PIN_OUTPUT,
	[CHIP_74244_Y13 ] = CHIP_PIN_OUTPUT,
	[CHIP_74244_Y14 ] = CHIP_PIN_OUTPUT,
	[CHIP_74244_Y21 ] = CHIP_PIN_OUTPUT,
	[CHIP_74244_Y22 ] = CHIP_PIN_OUTPUT,
	[CHIP_74244_Y23 ] = CHIP_PIN_OUTPUT,
	[CHIP_74244_Y24 ] = CHIP_PIN_OUTPUT,
};

static void chip_74244_octal_buffer_destroy(Chip74244OctalBuffer *chip);
static void chip_74244_octal_buffer_process(Chip74244OctalBuffer *chip);

Chip74244OctalBuffer *chip_74244_octal_buffer_create(Simulator *sim, Chip74244Signals signals) {
	Chip74244OctalBuffer *chip = (Chip74244OctalBuffer *) calloc(1, sizeof(Chip74244OctalBuffer));
	CHIP_74XXX_SET_FUNCTIONS(chip_74244_octal_buffer);
	CHIP_74XXX_SET_VARIABLES(Chip74244_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74244Signals));
	SIGNAL_DEFINE(G1_B);
	SIGNAL_DEFINE(A11);
	SIGNAL_DEFINE(Y24);
	SIGNAL_DEFINE(A12);
	SIGNAL_DEFINE(Y23);
	SIGNAL_DEFINE(A13);
	SIGNAL_DEFINE(Y22);
	SIGNAL_DEFINE(A14);
	SIGNAL_DEFINE(Y21);
	SIGNAL_DEFINE(A21);
	SIGNAL_DEFINE(Y14);
	SIGNAL_DEFINE(A22);
	SIGNAL_DEFINE(Y13);
	SIGNAL_DEFINE(A23);
	SIGNAL_DEFINE(Y12);
	SIGNAL_DEFINE(A24);
	SIGNAL_DEFINE(Y11);
	SIGNAL_DEFINE(G2_B);

	return chip;
}

static void chip_74244_octal_buffer_destroy(Chip74244OctalBuffer *chip) {
	assert(chip);
	free(chip);
}

static void chip_74244_octal_buffer_process(Chip74244OctalBuffer *chip) {
	assert(chip);

	if (!SIGNAL_READ(G1_B)) {
		SIGNAL_WRITE(Y11, SIGNAL_READ(A11));
		SIGNAL_WRITE(Y12, SIGNAL_READ(A12));
		SIGNAL_WRITE(Y13, SIGNAL_READ(A13));
		SIGNAL_WRITE(Y14, SIGNAL_READ(A14));
	} else if (SIGNAL_CHANGED(G1_B)) {
		SIGNAL_NO_WRITE(Y11);
		SIGNAL_NO_WRITE(Y12);
		SIGNAL_NO_WRITE(Y13);
		SIGNAL_NO_WRITE(Y14);
	}

	if (!SIGNAL_READ(G2_B)) {
		SIGNAL_WRITE(Y21, SIGNAL_READ(A21));
		SIGNAL_WRITE(Y22, SIGNAL_READ(A22));
		SIGNAL_WRITE(Y23, SIGNAL_READ(A23));
		SIGNAL_WRITE(Y24, SIGNAL_READ(A24));
	} else if (SIGNAL_CHANGED(G2_B)) {
		SIGNAL_NO_WRITE(Y21);
		SIGNAL_NO_WRITE(Y22);
		SIGNAL_NO_WRITE(Y23);
		SIGNAL_NO_WRITE(Y24);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// 74373 - Octal D-Type Transparant Latches
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_74373_

static uint8_t Chip74373_PinTypes[CHIP_74373_PIN_COUNT] = {
	[CHIP_74373_OC_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74373_C   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74373_D1  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74373_D2  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74373_D3  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74373_D4  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74373_D5  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74373_D6  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74373_D7  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74373_D8  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_74373_Q1  ] = CHIP_PIN_OUTPUT,
	[CHIP_74373_Q2  ] = CHIP_PIN_OUTPUT,
	[CHIP_74373_Q3  ] = CHIP_PIN_OUTPUT,
	[CHIP_74373_Q4  ] = CHIP_PIN_OUTPUT,
	[CHIP_74373_Q5  ] = CHIP_PIN_OUTPUT,
	[CHIP_74373_Q6  ] = CHIP_PIN_OUTPUT,
	[CHIP_74373_Q7  ] = CHIP_PIN_OUTPUT,
	[CHIP_74373_Q8  ] = CHIP_PIN_OUTPUT,
};

static void chip_74373_latch_destroy(Chip74373Latch *chip);
static void chip_74373_latch_process(Chip74373Latch *chip);

Chip74373Latch *chip_74373_latch_create(Simulator *sim, Chip74373Signals signals) {
	Chip74373Latch *chip = (Chip74373Latch *) calloc(1, sizeof(Chip74373Latch));
	CHIP_74XXX_SET_FUNCTIONS(chip_74373_latch);
	CHIP_74XXX_SET_VARIABLES(Chip74373_PinTypes)

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Chip74373Signals));
	SIGNAL_DEFINE(OC_B);
	SIGNAL_DEFINE(Q1);
	SIGNAL_DEFINE(D1);
	SIGNAL_DEFINE(D2);
	SIGNAL_DEFINE(Q2);
	SIGNAL_DEFINE(Q3);
	SIGNAL_DEFINE(D3);
	SIGNAL_DEFINE(D4);
	SIGNAL_DEFINE(Q4);
	SIGNAL_DEFINE(C);
	SIGNAL_DEFINE(Q5);
	SIGNAL_DEFINE(D5);
	SIGNAL_DEFINE(D6);
	SIGNAL_DEFINE(Q6);
	SIGNAL_DEFINE(Q7);
	SIGNAL_DEFINE(D7);
	SIGNAL_DEFINE(D8);
	SIGNAL_DEFINE(Q8);

	return chip;
}

static void chip_74373_latch_destroy(Chip74373Latch *chip) {
	assert(chip);
	free(chip);
}

static void chip_74373_latch_process(Chip74373Latch *chip) {
	if (SIGNAL_READ(C)) {
		chip->state = (uint8_t)	(
				(SIGNAL_READ(D1) << 0) |
				(SIGNAL_READ(D2) << 1) |
				(SIGNAL_READ(D3) << 2) |
				(SIGNAL_READ(D4) << 3) |
				(SIGNAL_READ(D5) << 4) |
				(SIGNAL_READ(D6) << 5) |
				(SIGNAL_READ(D7) << 6) |
				(SIGNAL_READ(D8) << 7)
		);
	}

	if (!SIGNAL_READ(OC_B)) {
		SIGNAL_WRITE(Q1, (chip->state >> 0) & 0x01);
		SIGNAL_WRITE(Q2, (chip->state >> 1) & 0x01);
		SIGNAL_WRITE(Q3, (chip->state >> 2) & 0x01);
		SIGNAL_WRITE(Q4, (chip->state >> 3) & 0x01);
		SIGNAL_WRITE(Q5, (chip->state >> 4) & 0x01);
		SIGNAL_WRITE(Q6, (chip->state >> 5) & 0x01);
		SIGNAL_WRITE(Q7, (chip->state >> 6) & 0x01);
		SIGNAL_WRITE(Q8, (chip->state >> 7) & 0x01);
	} else {
		SIGNAL_NO_WRITE(Q1);
		SIGNAL_NO_WRITE(Q2);
		SIGNAL_NO_WRITE(Q3);
		SIGNAL_NO_WRITE(Q4);
		SIGNAL_NO_WRITE(Q5);
		SIGNAL_NO_WRITE(Q6);
		SIGNAL_NO_WRITE(Q7);
		SIGNAL_NO_WRITE(Q8);
	}
}
