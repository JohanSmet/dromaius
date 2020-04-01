// chip_74xxx.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of 74-family logic chips

#ifndef DROMAIUS_CHIP_74XXX_H
#define DROMAIUS_CHIP_74XXX_H

#include "types.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct Chip7400NandSignals {
	Signal		a1;			// pin 1
	Signal		b1;			// pin 2
	Signal		y1;			// pin 3
	Signal		a2;			// pin 4
	Signal		b2;			// pin 5
	Signal		y2;			// pin 6
	Signal		gnd;		// pin 7
	Signal		y3;			// pin 8
	Signal		a3;			// pin 9
	Signal		b3;			// pin 10
	Signal		y4;			// pin 11
	Signal		a4;			// pin 12
	Signal		b4;			// pin 13
	Signal		vcc;		// pin 14
} Chip7400NandSignals;

typedef struct Chip7400Nand {
	SignalPool *		signal_pool;
	Chip7400NandSignals signals;
} Chip7400Nand;

// functions
Chip7400Nand *chip_7400_nand_create(SignalPool *signal_pool, Chip7400NandSignals signals);
void chip_7400_nand_destroy(Chip7400Nand *chip);
void chip_7400_nand_process(Chip7400Nand *chip);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_74XXX_H
