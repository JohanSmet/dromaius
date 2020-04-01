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

typedef struct Chip74244OSignals {
	Signal		g1_b;		// pin 01 - enable output on group 1
	Signal		a11;		// pin 02 - group 1 input 1
	Signal		y24;		// pin 03 - group 2 output 4
	Signal		a12;		// pin 04 - group 1 input 2
	Signal		y23;		// pin 05 - group 2 output 3
	Signal		a13;		// pin 06 - group 1 input 3
	Signal		y22;		// pin 07 - group 2 output 2
	Signal		a14;		// pin 08 - group 1 input 4
	Signal		y21;		// pin 09 - group 2 output 1
	Signal		gnd;		// pin 10
	Signal		a21;		// pin 11 - group 2 input 1
	Signal		y14;		// pin 12 - group 1 output 4
	Signal		a22;		// pin 13 - group 2 input 2
	Signal		y13;		// pin 14 - group 1 output 3
	Signal		a23;		// pin 15 - group 2 input 3
	Signal		y12;		// pin 16 - group 1 output 2
	Signal		a24;		// pin 17 - group 2 input 4
	Signal		y11;		// pin 18 - group 1 output 1
	Signal		g2_b;		// pin 19 - enable output on group 2
	Signal		vcc;		// pin 20
} Chip74244Signals;

typedef struct Chip74244OctalBuffer {
	SignalPool *		signal_pool;
	Chip74244Signals signals;
} Chip74244OctalBuffer;

// functions
Chip7400Nand *chip_7400_nand_create(SignalPool *signal_pool, Chip7400NandSignals signals);
void chip_7400_nand_destroy(Chip7400Nand *chip);
void chip_7400_nand_process(Chip7400Nand *chip);

Chip74244OctalBuffer *chip_74244_octal_buffer_create(SignalPool *signal_pool, Chip74244Signals signals);
void chip_74244_octal_buffer_destroy(Chip74244OctalBuffer *chip);
void chip_74244_octal_buffer_process(Chip74244OctalBuffer *chip);



#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_74XXX_H
