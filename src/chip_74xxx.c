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
