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

// types - 7400
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

// types - 74145
typedef struct Chip74145Signals {
	Signal		y0_b;		// pin 1
	Signal		y1_b;		// pin 2
	Signal		y2_b;		// pin 3
	Signal		y3_b;		// pin 4
	Signal		y4_b;		// pin 5
	Signal		y5_b;		// pin 6
	Signal		y6_b;		// pin 7
	Signal		gnd;		// pin 8
	Signal		y7_b;		// pin 9
	Signal		y8_b;		// pin 10
	Signal		y9_b;		// pin 11
	Signal		d;			// pin 12
	Signal		c;			// pin 13
	Signal		b;			// pin 14
	Signal		a;			// pin 15
	Signal		vcc;		// pin 16
} Chip74145Signals;

typedef struct Chip74145BcdDecoder {
	SignalPool *		signal_pool;
	Chip74145Signals	signals;
} Chip74145BcdDecoder;

// types - 74154
typedef struct Chip74154Signals {
	Signal		y0_b;		// pin 1
	Signal		y1_b;		// pin 2
	Signal		y2_b;		// pin 3
	Signal		y3_b;		// pin 4
	Signal		y4_b;		// pin 5
	Signal		y5_b;		// pin 6
	Signal		y6_b;		// pin 7
	Signal		y7_b;		// pin 8
	Signal		y8_b;		// pin 9
	Signal		y9_b;		// pin 10
	Signal		y10_b;		// pin 11
	Signal		gnd;		// pin 12
	Signal		y11_b;		// pin 13
	Signal		y12_b;		// pin 14
	Signal		y13_b;		// pin 15
	Signal		y14_b;		// pin 16
	Signal		y15_b;		// pin 17
	Signal		g1_b;		// pin 18
	Signal		g2_b;		// pin 19
	Signal		d;			// pin 20
	Signal		c;			// pin 21
	Signal		b;			// pin 22
	Signal		a;			// pin 23
	Signal		vcc;		// pin 24
} Chip74154Signals;

typedef struct Chip74154Decoder {
	SignalPool *		signal_pool;
	Chip74154Signals	signals;
} Chip74154Decoder;

// types - 74164 (8-Bit Serial In/Parallel Out Shift Register)
typedef struct Chip74164Signals {
	Signal		a;			// pin 01 - serial input A
	Signal		b;			// pin 02 - serial input B
	Signal		qa;			// pin 03 - parallel output A
	Signal		qb;			// pin 04 - parallel output B
	Signal		qc;			// pin 05 - parallel output C
	Signal		qd;			// pin 06 - parallel output D
	Signal		gnd;		// pin 07
	Signal		clk;		// pin 08 - clock
	Signal		clear_b;	// pin 09 - reset
	Signal		qe;			// pin 10 - parallel output A
	Signal		qf;			// pin 11 - parallel output B
	Signal		qg;			// pin 12 - parallel output C
	Signal		qh;			// pin 13 - parallel output D
	Signal		vcc;		// pin 14
} Chip74164Signals;

typedef struct Chip74164ShiftRegister {
	SignalPool *		signal_pool;
	Chip74164Signals	signals;

	uint8_t				state;
	bool				prev_clock;
} Chip74164ShiftRegister;

// types - 74244
typedef struct Chip74244Signals {
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

Chip74145BcdDecoder *chip_74145_bcd_decoder_create(SignalPool *signal_pool, Chip74145Signals signals);
void chip_74145_bcd_decoder_destroy(Chip74145BcdDecoder *chip);
void chip_74145_bcd_decoder_process(Chip74145BcdDecoder *chip);

Chip74154Decoder *chip_74154_decoder_create(SignalPool *signal_pool, Chip74154Signals signals);
void chip_74154_decoder_destroy(Chip74154Decoder *chip);
void chip_74154_decoder_process(Chip74154Decoder *chip);

Chip74164ShiftRegister *chip_74164_shift_register_create(SignalPool *signal_pool, Chip74164Signals signals);
void chip_74164_shift_register_destroy(Chip74164ShiftRegister *chip);
void chip_74164_shift_register_process(Chip74164ShiftRegister *chip);

Chip74244OctalBuffer *chip_74244_octal_buffer_create(SignalPool *signal_pool, Chip74244Signals signals);
void chip_74244_octal_buffer_destroy(Chip74244OctalBuffer *chip);
void chip_74244_octal_buffer_process(Chip74244OctalBuffer *chip);



#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_74XXX_H
