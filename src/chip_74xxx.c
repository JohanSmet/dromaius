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
	assert(chip);
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
