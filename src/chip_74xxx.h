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

// types - 7474
typedef struct Chip7474Signals {
	Signal		clr1_b;		// pin 1
	Signal		d1;			// pin 2
	Signal		clk1;		// pin 3
	Signal		pr1_b;		// pin 4
	Signal		q1;			// pin 5
	Signal		q1_b;		// pin 6
	Signal		gnd;		// pin 7
	Signal		q2_b;		// pin 8
	Signal		q2;			// pin 9
	Signal		pr2_b;		// pin 10
	Signal		clk2;		// pin 11
	Signal		d2;			// pin 12
	Signal		clr2_b;		// pin 13
	Signal		vcc;		// pin 14
} Chip7474Signals;

typedef struct Chip7474DFlipFlop {
	SignalPool *		signal_pool;
	Chip7474Signals		signals;

	bool				prev_clk1;
	bool				prev_clk2;
	bool				q1;
	bool				q1_b;
	bool				q2;
	bool				q2_b;
} Chip7474DFlipFlop;

// types - 7493 (4-Bit Binary Counter)
typedef struct Chip7493Signals {
	Signal		b_b;		// pin 1
	Signal		r01;		// pin 2
	Signal		r02;		// pin 3
	Signal		vcc;		// pin 5
	Signal		qc;			// pin 8
	Signal		qb;			// pin 9
	Signal		gnd;		// pin 10
	Signal		qd;			// pin 11
	Signal		qa;			// pin 12
	Signal		a_b;		// pin 14
} Chip7493Signals;

typedef struct Chip7493BinaryCounter {
	SignalPool *		signal_pool;
	Chip7493Signals		signals;

	bool				count_a;
	int					count_b;
	bool				prev_a_b;
	bool				prev_b_b;
} Chip7493BinaryCounter;

// types - 74107
typedef struct Chip74107Signals {
	Signal		j1;			// pin 1
	Signal		q1_b;		// pin 2
	Signal		q1;			// pin 3
	Signal		k1;			// pin 4
	Signal		q2;			// pin 5
	Signal		q2_b;		// pin 6
	Signal		gnd;		// pin 7
	Signal		j2;			// pin 8
	Signal		clk2;		// pin 9
	Signal		clr2_b;		// pin 10
	Signal		k2;			// pin 11
	Signal		clk1;		// pin 12
	Signal		clr1_b;		// pin 13
	Signal		vcc;		// pin 14
} Chip74107Signals;

typedef struct Chip74107JKFlipFlop {
	SignalPool *		signal_pool;
	Chip74107Signals	signals;

	bool				prev_clk1;
	bool				prev_clk2;
	bool				q1;
	bool				q2;
} Chip74107JKFlipFlop;

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

// types - 74157 (Quad 2-Input Multiplexer)
typedef struct Chip74157Signals {
	Signal		sel;		// pin 01 - common select input
	Signal		i0a;		// pin 02 - input 0 a
	Signal		i1a;		// pin 03 - input 1 a
	Signal		za;			// pin 04 - output a
	Signal		i0b;		// pin 05 - input 0 b
	Signal		i1b;		// pin 06 - input 1 b
	Signal		zb;			// pin 07 - output b
	Signal		gnd;		// pin 08
	Signal		zd;			// pin 09 - output d
	Signal		i1d;		// pin 10 - input 1 d
	Signal		i0d;		// pin 11 - input 0 d
	Signal		zc;			// pin 12 - output c
	Signal		i1c;		// pin 13 - input 1 c
	Signal		i0c;		// pin 14 - input 0 c
	Signal		enable_b;	// pin 15 - chip enable
	Signal		vcc;		// pin 16
} Chip74157Signals;

typedef struct Chip74157Multiplexer {
	SignalPool *		signal_pool;
	Chip74157Signals	signals;
} Chip74157Multiplexer;

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

// types - 74191 (4-Bit Synchronous Up/Down Binary Counter)
typedef struct Chip74191Signals {
	Signal		b;			// pin 01
	Signal		qb;			// pin 02
	Signal		qa;			// pin 03
	Signal		enable_b;	// pin 04
	Signal		d_u;		// pin 05
	Signal		qc;			// pin 06
	Signal		qd;			// pin 07
	Signal		gnd;		// pin 08
	Signal		d;			// pin 09
	Signal		c;			// pin 10
	Signal		load_b;		// pin 11
	Signal		max_min;	// pin 12
	Signal		rco_b;		// pin 13
	Signal		clk;		// pin 14
	Signal		a;			// pin 15
	Signal		vcc;		// pin 16
} Chip74191Signals;

typedef struct Chip74191BinaryCounter {
	SignalPool *		signal_pool;
	Chip74191Signals	signals;

	int					state;
	bool				max_min;
	bool				prev_clock;
} Chip74191BinaryCounter;

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

Chip7474DFlipFlop *chip_7474_d_flipflop_create(SignalPool *signal_pool, Chip7474Signals signals);
void chip_7474_d_flipflop_destroy(Chip7474DFlipFlop *chip);
void chip_7474_d_flipflop_process(Chip7474DFlipFlop *chip);

Chip7493BinaryCounter *chip_7493_binary_counter_create(SignalPool *signal_pool, Chip7493Signals signals);
void chip_7493_binary_counter_destroy(Chip7493BinaryCounter *chip);
void chip_7493_binary_counter_process(Chip7493BinaryCounter *chip);

Chip74107JKFlipFlop *chip_74107_jk_flipflop_create(SignalPool *signal_pool, Chip74107Signals signals);
void chip_74107_jk_flipflop_destroy(Chip74107JKFlipFlop *chip);
void chip_74107_jk_flipflop_process(Chip74107JKFlipFlop *chip);

Chip74145BcdDecoder *chip_74145_bcd_decoder_create(SignalPool *signal_pool, Chip74145Signals signals);
void chip_74145_bcd_decoder_destroy(Chip74145BcdDecoder *chip);
void chip_74145_bcd_decoder_process(Chip74145BcdDecoder *chip);

Chip74154Decoder *chip_74154_decoder_create(SignalPool *signal_pool, Chip74154Signals signals);
void chip_74154_decoder_destroy(Chip74154Decoder *chip);
void chip_74154_decoder_process(Chip74154Decoder *chip);

Chip74157Multiplexer *chip_74157_multiplexer_create(SignalPool *signal_pool, Chip74157Signals signals);
void chip_74157_multiplexer_destroy(Chip74157Multiplexer *chip);
void chip_74157_multiplexer_process(Chip74157Multiplexer *chip);

Chip74164ShiftRegister *chip_74164_shift_register_create(SignalPool *signal_pool, Chip74164Signals signals);
void chip_74164_shift_register_destroy(Chip74164ShiftRegister *chip);
void chip_74164_shift_register_process(Chip74164ShiftRegister *chip);

Chip74191BinaryCounter *chip_74191_binary_counter_create(SignalPool *signal_pool, Chip74191Signals signals);
void chip_74191_binary_counter_destroy(Chip74191BinaryCounter *chip);
void chip_74191_binary_counter_process(Chip74191BinaryCounter *chip);

Chip74244OctalBuffer *chip_74244_octal_buffer_create(SignalPool *signal_pool, Chip74244Signals signals);
void chip_74244_octal_buffer_destroy(Chip74244OctalBuffer *chip);
void chip_74244_octal_buffer_process(Chip74244OctalBuffer *chip);



#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_74XXX_H
