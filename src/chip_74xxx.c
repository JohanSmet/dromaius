// chip_74xxx.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of 74-family logic chips

#include "chip_74xxx.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			chip->signal_pool
#define SIGNAL_COLLECTION	chip->signals

///////////////////////////////////////////////////////////////////////////////
//
// 7400 - Quad 2 Input NAND gates
//

Chip7400Nand *chip_7400_nand_create(SignalPool *signal_pool, Chip7400NandSignals signals) {
	Chip7400Nand *chip = (Chip7400Nand *) calloc(1, sizeof(Chip7400Nand));
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(a1,	1);
	SIGNAL_DEFINE(b1,	1);
	SIGNAL_DEFINE(y1,	1);
	SIGNAL_DEFINE(a2,	1);
	SIGNAL_DEFINE(b2,	1);
	SIGNAL_DEFINE(y2,	1);
	SIGNAL_DEFINE(a3,	1);
	SIGNAL_DEFINE(b3,	1);
	SIGNAL_DEFINE(y3,	1);
	SIGNAL_DEFINE(a4,	1);
	SIGNAL_DEFINE(b4,	1);
	SIGNAL_DEFINE(y4,	1);

	return chip;
}

void chip_7400_nand_destroy(Chip7400Nand *chip) {
	assert(chip);
	free(chip);
}

void chip_7400_nand_process(Chip7400Nand *chip) {
	assert(chip);

	SIGNAL_SET_BOOL(y1, !(SIGNAL_NEXT_BOOL(a1) && SIGNAL_NEXT_BOOL(b1)));
	SIGNAL_SET_BOOL(y2, !(SIGNAL_NEXT_BOOL(a2) && SIGNAL_NEXT_BOOL(b2)));
	SIGNAL_SET_BOOL(y3, !(SIGNAL_NEXT_BOOL(a3) && SIGNAL_NEXT_BOOL(b3)));
	SIGNAL_SET_BOOL(y4, !(SIGNAL_NEXT_BOOL(a4) && SIGNAL_NEXT_BOOL(b4)));
}

///////////////////////////////////////////////////////////////////////////////
//
// 7474 - Dual Positive-Edge-Triggered D Flip-Flops with preset/clear
//

Chip7474DFlipFlop *chip_7474_d_flipflop_create(SignalPool *signal_pool, Chip7474Signals signals) {
	Chip7474DFlipFlop *chip = (Chip7474DFlipFlop *) calloc(1, sizeof(Chip7474DFlipFlop));
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE_BOOL(clr1_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE(d1, 1);
	SIGNAL_DEFINE(clk1, 1);
	SIGNAL_DEFINE_BOOL(pr1_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE(q1, 1);
	SIGNAL_DEFINE(q1_b, 1);
	SIGNAL_DEFINE_BOOL(gnd, 1, false);
	SIGNAL_DEFINE(q2_b, 1);
	SIGNAL_DEFINE(q2, 1);
	SIGNAL_DEFINE_BOOL(pr2_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE(clk2, 1);
	SIGNAL_DEFINE(d2, 1);
	SIGNAL_DEFINE_BOOL(clr2_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(vcc, 1, true);

	return chip;
}

void chip_7474_d_flipflop_destroy(Chip7474DFlipFlop *chip) {
	assert(chip);
	free(chip);
}

void chip_7474_d_flipflop_process(Chip7474DFlipFlop *chip) {
	assert(chip);

	// flip-flop 1
	bool clk1 = SIGNAL_BOOL(clk1);

	if (ACTLO_ASSERTED(SIGNAL_BOOL(pr1_b)) && ACTLO_ASSERTED(SIGNAL_BOOL(clr1_b))) {
		chip->q1 = true;
		chip->q1_b = true;
	} else if (ACTLO_ASSERTED(SIGNAL_BOOL(pr1_b))) {
		chip->q1 = true;
		chip->q1_b = false;
	} else if (ACTLO_ASSERTED(SIGNAL_BOOL(clr1_b))) {
		chip->q1 = false;
		chip->q1_b = true;
	} else if (clk1 && !chip->prev_clk1) {
		chip->q1 = SIGNAL_BOOL(d1);
		chip->q1_b = !chip->q1;
	}

	SIGNAL_SET_BOOL(q1, chip->q1);
	SIGNAL_SET_BOOL(q1_b, chip->q1_b);
	chip->prev_clk1 = clk1;

	// flip-flop 2
	bool clk2 = SIGNAL_BOOL(clk2);

	if (ACTLO_ASSERTED(SIGNAL_BOOL(pr2_b)) && ACTLO_ASSERTED(SIGNAL_BOOL(clr2_b))) {
		chip->q2 = true;
		chip->q2_b = true;
	} else if (ACTLO_ASSERTED(SIGNAL_BOOL(pr2_b))) {
		chip->q2 = true;
		chip->q2_b = false;
	} else if (ACTLO_ASSERTED(SIGNAL_BOOL(clr2_b))) {
		chip->q2 = false;
		chip->q2_b = true;
	} else if (clk2 && !chip->prev_clk2) {
		chip->q2 = SIGNAL_BOOL(d2);
		chip->q2_b = !chip->q2;
	}

	SIGNAL_SET_BOOL(q2, chip->q2);
	SIGNAL_SET_BOOL(q2_b, chip->q2_b);
	chip->prev_clk2 = clk2;
}

///////////////////////////////////////////////////////////////////////////////
//
// 7493 - 4-Bit Binary Counter
//

Chip7493BinaryCounter *chip_7493_binary_counter_create(SignalPool *signal_pool, Chip7493Signals signals) {
	Chip7493BinaryCounter *chip = (Chip7493BinaryCounter *) calloc(1, sizeof(Chip7493BinaryCounter));
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(b_b, 1);					// pin 1
	SIGNAL_DEFINE_BOOL(r01, 1, false);		// pin 2
	SIGNAL_DEFINE_BOOL(r02, 1, false);		// pin 3
	SIGNAL_DEFINE_BOOL(vcc, 1, true);		// pin 5
	SIGNAL_DEFINE(qc, 1);					// pin 8
	SIGNAL_DEFINE(qb, 1);					// pin 9
	SIGNAL_DEFINE_BOOL(gnd, 1, false);		// pin 10
	SIGNAL_DEFINE(qd, 1);					// pin 11
	SIGNAL_DEFINE(qa, 1);					// pin 12
	SIGNAL_DEFINE(a_b, 1);					// pin 14

	return chip;
}

void chip_7493_binary_counter_destroy(Chip7493BinaryCounter *chip) {
	assert(chip);
	free(chip);
}

void chip_7493_binary_counter_process(Chip7493BinaryCounter *chip) {
	assert(chip);

	bool a_b = SIGNAL_BOOL(a_b);
	bool b_b = SIGNAL_BOOL(b_b);

	if (ACTHI_ASSERTED(SIGNAL_BOOL(r01)) && ACTHI_ASSERTED(SIGNAL_BOOL(r02))) {
		chip->count_a = 0;
		chip->count_b = 0;
	} else {
		if (chip->prev_a_b && !a_b) {
			chip->count_a = !chip->count_a;
		}

		// ok, a bit hacky ... if the second stage's clock is connected to the output of the first stage:
		// don't wait until the next process call to trigger the second stage
		if (memcmp(&SIGNAL(b_b), &SIGNAL(qa), sizeof(Signal)) == 0) {
			if (chip->prev_a_b && !a_b) {
				chip->count_b += !chip->count_a;
			}
		} else if (chip->prev_b_b && !b_b) {
			++chip->count_b;
		}
	}

	SIGNAL_SET_BOOL(qa, chip->count_a);
	SIGNAL_SET_BOOL(qb, chip->count_b & 0b0001);
	SIGNAL_SET_BOOL(qc, (chip->count_b & 0b0010) >> 1);
	SIGNAL_SET_BOOL(qd, (chip->count_b & 0b0100) >> 2);

	// save the clock state
	chip->prev_a_b = a_b;
	chip->prev_b_b = b_b;
}

///////////////////////////////////////////////////////////////////////////////
//
// 74107 - Dual J-K Flip-Flops with clear
//

Chip74107JKFlipFlop *chip_74107_jk_flipflop_create(SignalPool *signal_pool, Chip74107Signals signals) {
	Chip74107JKFlipFlop *chip = (Chip74107JKFlipFlop *) calloc(1, sizeof(Chip74107JKFlipFlop));
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));

	SIGNAL_DEFINE(j1, 1);
	SIGNAL_DEFINE(q1_b, 1);
	SIGNAL_DEFINE(q1, 1);
	SIGNAL_DEFINE(k1, 1);
	SIGNAL_DEFINE(q2, 1);
	SIGNAL_DEFINE(q2_b, 1);
	SIGNAL_DEFINE_BOOL(gnd, 1, false);
	SIGNAL_DEFINE(j2, 1);
	SIGNAL_DEFINE(clk2, 1);
	SIGNAL_DEFINE_BOOL(clr2_b, 1, true);
	SIGNAL_DEFINE(k2, 1);
	SIGNAL_DEFINE(clk1, 1);
	SIGNAL_DEFINE_BOOL(clr1_b, 1, true);
	SIGNAL_DEFINE_BOOL(vcc, 1, true);

	return chip;
}

void chip_74107_jk_flipflop_destroy(Chip74107JKFlipFlop *chip) {
	assert(chip);
	free(chip);
}

void chip_74107_jk_flipflop_process(Chip74107JKFlipFlop *chip) {
	assert(chip);

	// flip-flop 1
	bool clk1 = SIGNAL_BOOL(clk1);

	if (ACTLO_ASSERTED(SIGNAL_BOOL(clr1_b))) {
		chip->q1 = false;
	} else if (clk1 && !chip->prev_clk1) {
		if (SIGNAL_BOOL(j1) && SIGNAL_BOOL(k1)) {
			chip->q1 = !chip->q1;
		} else if (SIGNAL_BOOL(j1) || SIGNAL_BOOL(k1)) {
			chip->q1 = SIGNAL_BOOL(j1);
		}
	}

	SIGNAL_SET_BOOL(q1, chip->q1);
	SIGNAL_SET_BOOL(q1_b, !chip->q1);
	chip->prev_clk1 = clk1;

	// flip-flop 2
	bool clk2 = SIGNAL_BOOL(clk2);

	if (ACTLO_ASSERTED(SIGNAL_BOOL(clr2_b))) {
		chip->q2 = false;
	} else if (clk2 && !chip->prev_clk2) {
		if (SIGNAL_BOOL(j2) && SIGNAL_BOOL(k2)) {
			chip->q2 = !chip->q2;
		} else if (SIGNAL_BOOL(j2) || SIGNAL_BOOL(k2)) {
			chip->q2 = SIGNAL_BOOL(j2);
		}
	}

	SIGNAL_SET_BOOL(q2, chip->q2);
	SIGNAL_SET_BOOL(q2_b, !chip->q2);
	chip->prev_clk2 = clk2;

	SIGNAL_SET_BOOL(q2, chip->q2);
	SIGNAL_SET_BOOL(q2_b, !chip->q2);
}

///////////////////////////////////////////////////////////////////////////////
//
// 74145 - BCD-to-Decimal Decoder/driver
//

typedef struct Chip74145BcdDecoder_private {
	Chip74145BcdDecoder	intf;
	Signal *			outputs[10];
} Chip74145BcdDecoder_private;

Chip74145BcdDecoder *chip_74145_bcd_decoder_create(SignalPool *signal_pool, Chip74145Signals signals) {
	Chip74145BcdDecoder_private *priv = (Chip74145BcdDecoder_private *) calloc(1, sizeof(Chip74145BcdDecoder_private));
	Chip74145BcdDecoder *chip = &priv->intf;
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(y0_b,		1);
	SIGNAL_DEFINE(y1_b,		1);
	SIGNAL_DEFINE(y2_b,		1);
	SIGNAL_DEFINE(y3_b,		1);
	SIGNAL_DEFINE(y4_b,		1);
	SIGNAL_DEFINE(y5_b,		1);
	SIGNAL_DEFINE(y6_b,		1);
	SIGNAL_DEFINE(gnd,		1);
	SIGNAL_DEFINE(y7_b,		1);
	SIGNAL_DEFINE(y8_b,		1);
	SIGNAL_DEFINE(y9_b,		1);
	SIGNAL_DEFINE(d,		1);
	SIGNAL_DEFINE(c,		1);
	SIGNAL_DEFINE(b,		1);
	SIGNAL_DEFINE(a,		1);
	SIGNAL_DEFINE(vcc,		1);

	priv->outputs[0]  = &SIGNAL(y0_b);	priv->outputs[1]  = &SIGNAL(y1_b);
	priv->outputs[2]  = &SIGNAL(y2_b);	priv->outputs[3]  = &SIGNAL(y3_b);
	priv->outputs[4]  = &SIGNAL(y4_b);	priv->outputs[5]  = &SIGNAL(y5_b);
	priv->outputs[6]  = &SIGNAL(y6_b);	priv->outputs[7]  = &SIGNAL(y7_b);
	priv->outputs[8]  = &SIGNAL(y8_b);	priv->outputs[9]  = &SIGNAL(y9_b);

	return chip;
}

void chip_74145_bcd_decoder_destroy(Chip74145BcdDecoder *chip) {
	assert((Chip74145BcdDecoder_private *) chip);
	free(chip);
}

void chip_74145_bcd_decoder_process(Chip74145BcdDecoder *chip) {
	assert(chip);

	Signal **outputs = ((Chip74145BcdDecoder_private *) chip)->outputs;

	for (int i = 0; i < 10; ++i) {
		signal_write_bool(chip->signal_pool, *outputs[i], ACTLO_DEASSERT);
	}

	int value = SIGNAL_NEXT_BOOL(a) |
				SIGNAL_NEXT_BOOL(b) << 1 |
				SIGNAL_NEXT_BOOL(c) << 2 |
				SIGNAL_NEXT_BOOL(d) << 3;

	if (value < 10) {
		signal_write_bool(chip->signal_pool, *outputs[value], ACTLO_ASSERT);
	}

}

///////////////////////////////////////////////////////////////////////////////
//
// 74154 - 4-Line to 16-Line Decoder/Multiplexer
//

typedef struct Chip74154Decoder_private {
	Chip74154Decoder	intf;
	Signal *			outputs[16];
} Chip74154Decoder_private;

Chip74154Decoder *chip_74154_decoder_create(SignalPool *signal_pool, Chip74154Signals signals) {
	Chip74154Decoder_private *priv = (Chip74154Decoder_private *) calloc(1, sizeof(Chip74154Decoder_private));
	Chip74154Decoder *chip = &priv->intf;
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(y0_b,		1);
	SIGNAL_DEFINE(y1_b,		1);
	SIGNAL_DEFINE(y2_b,		1);
	SIGNAL_DEFINE(y3_b,		1);
	SIGNAL_DEFINE(y4_b,		1);
	SIGNAL_DEFINE(y5_b,		1);
	SIGNAL_DEFINE(y6_b,		1);
	SIGNAL_DEFINE(y7_b,		1);
	SIGNAL_DEFINE(y8_b,		1);
	SIGNAL_DEFINE(y9_b,		1);
	SIGNAL_DEFINE(y10_b,	1);
	SIGNAL_DEFINE(gnd,		1);
	SIGNAL_DEFINE(y11_b,	1);
	SIGNAL_DEFINE(y12_b,	1);
	SIGNAL_DEFINE(y13_b,	1);
	SIGNAL_DEFINE(y14_b,	1);
	SIGNAL_DEFINE(y15_b,	1);
	SIGNAL_DEFINE(g1_b,		1);
	SIGNAL_DEFINE(g2_b,		1);
	SIGNAL_DEFINE(d,		1);
	SIGNAL_DEFINE(c,		1);
	SIGNAL_DEFINE(b,		1);
	SIGNAL_DEFINE(a,		1);
	SIGNAL_DEFINE(vcc,		1);

	priv->outputs[0]  = &SIGNAL(y0_b);	priv->outputs[1]  = &SIGNAL(y1_b);
	priv->outputs[2]  = &SIGNAL(y2_b);	priv->outputs[3]  = &SIGNAL(y3_b);
	priv->outputs[4]  = &SIGNAL(y4_b);	priv->outputs[5]  = &SIGNAL(y5_b);
	priv->outputs[6]  = &SIGNAL(y6_b);	priv->outputs[7]  = &SIGNAL(y7_b);
	priv->outputs[8]  = &SIGNAL(y8_b);	priv->outputs[9]  = &SIGNAL(y9_b);
	priv->outputs[10] = &SIGNAL(y10_b); priv->outputs[11] = &SIGNAL(y11_b);
	priv->outputs[12] = &SIGNAL(y12_b); priv->outputs[13] = &SIGNAL(y13_b);
	priv->outputs[14] = &SIGNAL(y14_b); priv->outputs[15] = &SIGNAL(y15_b);

	return chip;
}

void chip_74154_decoder_destroy(Chip74154Decoder *chip) {
	assert((Chip74154Decoder_private *) chip);
	free(chip);
}

void chip_74154_decoder_process(Chip74154Decoder *chip) {
	assert(chip);

	if (!(ACTLO_ASSERTED(SIGNAL_NEXT_BOOL(g1_b)) && ACTLO_ASSERTED(SIGNAL_NEXT_BOOL(g2_b)))) {
		return;
	}

	Signal **outputs = ((Chip74154Decoder_private *) chip)->outputs;

	for (int i = 0; i < 16; ++i) {
		signal_write_bool(chip->signal_pool, *outputs[i], ACTLO_DEASSERT);
	}

	int value = SIGNAL_NEXT_BOOL(a) |
				SIGNAL_NEXT_BOOL(b) << 1 |
				SIGNAL_NEXT_BOOL(c) << 2 |
				SIGNAL_NEXT_BOOL(d) << 3;
	signal_write_bool(chip->signal_pool, *outputs[value], ACTLO_ASSERT);
}

///////////////////////////////////////////////////////////////////////////////
//
// 74157 - Quad 2-Input Multiplexer
//

Chip74157Multiplexer *chip_74157_multiplexer_create(SignalPool *signal_pool, Chip74157Signals signals) {
	Chip74157Multiplexer *chip = (Chip74157Multiplexer *) calloc(1, sizeof(Chip74157Multiplexer));
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(sel, 1);
	SIGNAL_DEFINE(i0a, 1);
	SIGNAL_DEFINE(i1a, 1);
	SIGNAL_DEFINE(za, 1);
	SIGNAL_DEFINE(i0b, 1);
	SIGNAL_DEFINE(i1b, 1);
	SIGNAL_DEFINE(zb, 1);
	SIGNAL_DEFINE_BOOL(gnd, 1, false);
	SIGNAL_DEFINE(zd, 1);
	SIGNAL_DEFINE(i1d, 1);
	SIGNAL_DEFINE(i0d, 1);
	SIGNAL_DEFINE(zc, 1);
	SIGNAL_DEFINE(i1c, 1);
	SIGNAL_DEFINE(i0c, 1);
	SIGNAL_DEFINE(enable_b, 1);
	SIGNAL_DEFINE_BOOL(vcc, 1, true);

	return chip;
}

void chip_74157_multiplexer_destroy(Chip74157Multiplexer *chip) {
	assert(chip);
	free(chip);
}

void chip_74157_multiplexer_process(Chip74157Multiplexer *chip) {
	assert(chip);

	bool mask_0 = !SIGNAL_BOOL(enable_b) && !SIGNAL_BOOL(sel);
	bool mask_1 = !SIGNAL_BOOL(enable_b) && SIGNAL_BOOL(sel);

	SIGNAL_SET_BOOL(za, (SIGNAL_BOOL(i0a) && mask_0) || (SIGNAL_BOOL(i1a) && mask_1));
	SIGNAL_SET_BOOL(zb, (SIGNAL_BOOL(i0b) && mask_0) || (SIGNAL_BOOL(i1b) && mask_1));
	SIGNAL_SET_BOOL(zc, (SIGNAL_BOOL(i0c) && mask_0) || (SIGNAL_BOOL(i1c) && mask_1));
	SIGNAL_SET_BOOL(zd, (SIGNAL_BOOL(i0d) && mask_0) || (SIGNAL_BOOL(i1d) && mask_1));
}

///////////////////////////////////////////////////////////////////////////////
//
// 74164 - 8-Bit Serial In/Parallel Out Shift Register
//

Chip74164ShiftRegister *chip_74164_shift_register_create(SignalPool *signal_pool, Chip74164Signals signals) {
	Chip74164ShiftRegister *chip = (Chip74164ShiftRegister *) calloc(1, sizeof(Chip74164ShiftRegister));
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(a, 1);
	SIGNAL_DEFINE(b, 1);
	SIGNAL_DEFINE(qa, 1);
	SIGNAL_DEFINE(qb, 1);
	SIGNAL_DEFINE(qc, 1);
	SIGNAL_DEFINE(qd, 1);
	SIGNAL_DEFINE(gnd, 1);
	SIGNAL_DEFINE(clk, 1);
	SIGNAL_DEFINE_BOOL(clear_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE(qe, 1);
	SIGNAL_DEFINE(qf, 1);
	SIGNAL_DEFINE(qg, 1);
	SIGNAL_DEFINE(qh, 1);
	SIGNAL_DEFINE(vcc, 1);

	return chip;
}

void chip_74164_shift_register_destroy(Chip74164ShiftRegister *chip) {
	assert(chip);
	free(chip);
}

void chip_74164_shift_register_process(Chip74164ShiftRegister *chip) {
	assert(chip);

	bool clk = SIGNAL_BOOL(clk);

	if (ACTLO_ASSERTED(SIGNAL_BOOL(clear_b))) {
		chip->state = 0;
	} else if (clk && !chip->prev_clock) {
		// shift on the positive clock edge
		bool in = SIGNAL_BOOL(a) && SIGNAL_BOOL(b);
		chip->state = (uint8_t) (((chip->state << 1) | in) & 0xff);
	}

	// always output the signals
	SIGNAL_SET_BOOL(qa, (chip->state & 0b00000001) != 0);
	SIGNAL_SET_BOOL(qb, (chip->state & 0b00000010) != 0);
	SIGNAL_SET_BOOL(qc, (chip->state & 0b00000100) != 0);
	SIGNAL_SET_BOOL(qd, (chip->state & 0b00001000) != 0);
	SIGNAL_SET_BOOL(qe, (chip->state & 0b00010000) != 0);
	SIGNAL_SET_BOOL(qf, (chip->state & 0b00100000) != 0);
	SIGNAL_SET_BOOL(qg, (chip->state & 0b01000000) != 0);
	SIGNAL_SET_BOOL(qh, (chip->state & 0b10000000) != 0);

	// save the clock state
	chip->prev_clock = clk;
}

///////////////////////////////////////////////////////////////////////////////
//
// 74177 - Presettable Binary Counter/Latch
//

Chip74177BinaryCounter *chip_74177_binary_counter_create(SignalPool *signal_pool, Chip74177Signals signals) {
	Chip74177BinaryCounter *chip = (Chip74177BinaryCounter *) calloc(1, sizeof(Chip74177BinaryCounter));
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE_BOOL(load_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE(qc, 1);
	SIGNAL_DEFINE(c, 1);
	SIGNAL_DEFINE(a, 1);
	SIGNAL_DEFINE(qa, 1);
	SIGNAL_DEFINE(clk2, 1);
	SIGNAL_DEFINE_BOOL(gnd, 1, false);
	SIGNAL_DEFINE(clk1, 1);
	SIGNAL_DEFINE(qb, 1);
	SIGNAL_DEFINE(b, 1);
	SIGNAL_DEFINE(d, 1);
	SIGNAL_DEFINE(qd, 1);
	SIGNAL_DEFINE_BOOL(clear_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(vcc, 1, true);

	return chip;
}

void chip_74177_binary_counter_destroy(Chip74177BinaryCounter *chip) {
	assert(chip);
	free(chip);
}

void chip_74177_binary_counter_process(Chip74177BinaryCounter *chip) {
	assert(chip);

	bool clk1 = SIGNAL_BOOL(clk1);
	bool clk2 = SIGNAL_BOOL(clk2);

	if (ACTLO_ASSERTED(SIGNAL_BOOL(load_b))) {
		chip->count_1 = SIGNAL_BOOL(a);
		chip->count_2 = SIGNAL_BOOL(b) | (SIGNAL_BOOL(c) << 1) | (SIGNAL_BOOL(d) << 2);
	} else if (ACTLO_ASSERTED(SIGNAL_BOOL(clear_b))) {
		chip->count_1 = false;
		chip->count_2 = false;
	} else {
		if (chip->prev_clk1 && !clk1) {
			chip->count_1 = !chip->count_1;
		}

		// ok, a bit hacky ... if the second stage's clock is connected to the output of the first stage:
		// don't wait until the next process call to trigger the second stage
		if (memcmp(&SIGNAL(clk2), &SIGNAL(qa), sizeof(Signal)) == 0) {
			if (chip->prev_clk1 && !clk1) {
				chip->count_2 += !chip->count_1;
			}
		} else if (chip->prev_clk2 && !clk2) {
			++chip->count_2;
		}
	}

	SIGNAL_SET_BOOL(qa, chip->count_1);
	SIGNAL_SET_BOOL(qb, (chip->count_2 >> 0) & 0x01);
	SIGNAL_SET_BOOL(qc, (chip->count_2 >> 1) & 0x01);
	SIGNAL_SET_BOOL(qd, (chip->count_2 >> 2) & 0x01);

	chip->prev_clk1 = clk1;
	chip->prev_clk2 = clk2;
}

///////////////////////////////////////////////////////////////////////////////
//
// 74191 - 4-Bit Synchronous Up/Down Binary Counter
//

Chip74191BinaryCounter *chip_74191_binary_counter_create(SignalPool *signal_pool, Chip74191Signals signals) {

	Chip74191BinaryCounter *chip = (Chip74191BinaryCounter *) calloc(1, sizeof(Chip74191BinaryCounter));
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(b, 1);
	SIGNAL_DEFINE(qb, 1);
	SIGNAL_DEFINE(qa, 1);
	SIGNAL_DEFINE_BOOL(enable_b, 1, ACTLO_ASSERT);
	SIGNAL_DEFINE_BOOL(d_u, 1, ACTHI_DEASSERT);
	SIGNAL_DEFINE(qc, 1);
	SIGNAL_DEFINE(qd, 1);
	SIGNAL_DEFINE(gnd, 1);
	SIGNAL_DEFINE(d, 1);
	SIGNAL_DEFINE(c, 1);
	SIGNAL_DEFINE_BOOL(load_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE(max_min, 1);
	SIGNAL_DEFINE(rco_b, 1);
	SIGNAL_DEFINE(clk, 1);
	SIGNAL_DEFINE(a, 1);
	SIGNAL_DEFINE(vcc, 1);

	return chip;
}

void chip_74191_binary_counter_destroy(Chip74191BinaryCounter *chip) {
	assert(chip);
	free(chip);
}

void chip_74191_binary_counter_process(Chip74191BinaryCounter *chip) {
	assert(chip);

	bool clk = SIGNAL_BOOL(clk);
	bool d_u = SIGNAL_BOOL(d_u);
	bool rco = true;

	if (ACTLO_ASSERTED(SIGNAL_BOOL(load_b))) {
		chip->state = (SIGNAL_BOOL(a) << 0) |
					  (SIGNAL_BOOL(b) << 1) |
					  (SIGNAL_BOOL(c) << 2) |
					  (SIGNAL_BOOL(d) << 3);
	} else if (ACTLO_ASSERTED(SIGNAL_BOOL(enable_b)) && clk && !chip->prev_clock) {
		// count on the positive clock edge
		chip->state = (chip->state - d_u + !d_u) & 0xf;
		chip->max_min = (d_u && chip->state == 0) || (!d_u && chip->state == 0xf);
	} else if (!clk && chip->prev_clock) {
		// negative clock edge
		rco = !chip->max_min;
	}

	SIGNAL_SET_BOOL(qa, (chip->state & 0b00000001) != 0);
	SIGNAL_SET_BOOL(qb, (chip->state & 0b00000010) != 0);
	SIGNAL_SET_BOOL(qc, (chip->state & 0b00000100) != 0);
	SIGNAL_SET_BOOL(qd, (chip->state & 0b00001000) != 0);
	SIGNAL_SET_BOOL(max_min, chip->max_min);
	SIGNAL_SET_BOOL(rco_b, rco);

	// save the clock state
	chip->prev_clock = clk;
}

///////////////////////////////////////////////////////////////////////////////
//
// 74244 - Octal 3-STATE Buffer/Line Driver/Line Receiver
//

Chip74244OctalBuffer *chip_74244_octal_buffer_create(SignalPool *signal_pool, Chip74244Signals signals) {
	Chip74244OctalBuffer *chip = (Chip74244OctalBuffer *) calloc(1, sizeof(Chip74244OctalBuffer));
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(g1_b,	1);
	SIGNAL_DEFINE(a11,	1);
	SIGNAL_DEFINE(y24,	1);
	SIGNAL_DEFINE(a12,	1);
	SIGNAL_DEFINE(y23,	1);
	SIGNAL_DEFINE(a13,	1);
	SIGNAL_DEFINE(y22,	1);
	SIGNAL_DEFINE(a14,	1);
	SIGNAL_DEFINE(y21,	1);
	SIGNAL_DEFINE(gnd,	1);
	SIGNAL_DEFINE(a21,	1);
	SIGNAL_DEFINE(y14,	1);
	SIGNAL_DEFINE(a22,	1);
	SIGNAL_DEFINE(y13,	1);
	SIGNAL_DEFINE(a23,	1);
	SIGNAL_DEFINE(y12,	1);
	SIGNAL_DEFINE(a24,	1);
	SIGNAL_DEFINE(y11,	1);
	SIGNAL_DEFINE(g2_b,	1);
	SIGNAL_DEFINE(vcc,	1);

	return chip;
}

void chip_74244_octal_buffer_destroy(Chip74244OctalBuffer *chip) {
	assert(chip);
	free(chip);
}

void chip_74244_octal_buffer_process(Chip74244OctalBuffer *chip) {
	assert(chip);

	if (!SIGNAL_NEXT_BOOL(g1_b)) {
		SIGNAL_SET_BOOL(y11, SIGNAL_NEXT_BOOL(a11));
		SIGNAL_SET_BOOL(y12, SIGNAL_NEXT_BOOL(a12));
		SIGNAL_SET_BOOL(y13, SIGNAL_NEXT_BOOL(a13));
		SIGNAL_SET_BOOL(y14, SIGNAL_NEXT_BOOL(a14));
	}

	if (!SIGNAL_NEXT_BOOL(g2_b)) {
		SIGNAL_SET_BOOL(y21, SIGNAL_NEXT_BOOL(a21));
		SIGNAL_SET_BOOL(y22, SIGNAL_NEXT_BOOL(a22));
		SIGNAL_SET_BOOL(y23, SIGNAL_NEXT_BOOL(a23));
		SIGNAL_SET_BOOL(y24, SIGNAL_NEXT_BOOL(a24));
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// 74373 - Octal D-Type Transparant Latches
//

Chip74373Latch *chip_74373_latch_create(SignalPool *signal_pool, Chip74373Signals signals) {
	Chip74373Latch *chip = (Chip74373Latch *) calloc(1, sizeof(Chip74373Latch));
	chip->signal_pool = signal_pool;

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(oc_b, 1);
	SIGNAL_DEFINE(q1, 1);
	SIGNAL_DEFINE(d1, 1);
	SIGNAL_DEFINE(d2, 1);
	SIGNAL_DEFINE(q2, 1);
	SIGNAL_DEFINE(q3, 1);
	SIGNAL_DEFINE(d3, 1);
	SIGNAL_DEFINE(d4, 1);
	SIGNAL_DEFINE(q4, 1);
	SIGNAL_DEFINE_BOOL(gnd, 1, false);
	SIGNAL_DEFINE(c, 1);
	SIGNAL_DEFINE(q5, 1);
	SIGNAL_DEFINE(d5, 1);
	SIGNAL_DEFINE(d6, 1);
	SIGNAL_DEFINE(q6, 1);
	SIGNAL_DEFINE(q7, 1);
	SIGNAL_DEFINE(d7, 1);
	SIGNAL_DEFINE(d8, 1);
	SIGNAL_DEFINE(q8, 1);
	SIGNAL_DEFINE_BOOL(vcc, 1, true);

	return chip;
}

void chip_74373_latch_destroy(Chip74373Latch *chip) {
	assert(chip);
	free(chip);
}

void chip_74373_latch_process(Chip74373Latch *chip) {
	if (SIGNAL_NEXT_BOOL(c)) {
		chip->state = (uint8_t)	(
				(SIGNAL_NEXT_BOOL(d1) << 0) |
				(SIGNAL_NEXT_BOOL(d2) << 1) |
				(SIGNAL_NEXT_BOOL(d3) << 2) |
				(SIGNAL_NEXT_BOOL(d4) << 3) |
				(SIGNAL_NEXT_BOOL(d5) << 4) |
				(SIGNAL_NEXT_BOOL(d6) << 5) |
				(SIGNAL_NEXT_BOOL(d7) << 6) |
				(SIGNAL_NEXT_BOOL(d8) << 7)
		);
	}

	if (!SIGNAL_NEXT_BOOL(oc_b)) {
		SIGNAL_SET_BOOL(q1, (chip->state >> 0) & 0x01);
		SIGNAL_SET_BOOL(q2, (chip->state >> 1) & 0x01);
		SIGNAL_SET_BOOL(q3, (chip->state >> 2) & 0x01);
		SIGNAL_SET_BOOL(q4, (chip->state >> 3) & 0x01);
		SIGNAL_SET_BOOL(q5, (chip->state >> 4) & 0x01);
		SIGNAL_SET_BOOL(q6, (chip->state >> 5) & 0x01);
		SIGNAL_SET_BOOL(q7, (chip->state >> 6) & 0x01);
		SIGNAL_SET_BOOL(q8, (chip->state >> 7) & 0x01);
	}
}
