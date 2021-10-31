// chip_74xxx.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of 74-family logic chips

#ifndef DROMAIUS_CHIP_74XXX_H
#define DROMAIUS_CHIP_74XXX_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types - 7400
typedef enum {
	CHIP_7400_A1 = CHIP_PIN_01,
	CHIP_7400_B1 = CHIP_PIN_02,
	CHIP_7400_Y1 = CHIP_PIN_03,
	CHIP_7400_A2 = CHIP_PIN_04,
	CHIP_7400_B2 = CHIP_PIN_05,
	CHIP_7400_Y2 = CHIP_PIN_06,
	CHIP_7400_Y3 = CHIP_PIN_08,
	CHIP_7400_B3 = CHIP_PIN_09,
	CHIP_7400_A3 = CHIP_PIN_10,
	CHIP_7400_Y4 = CHIP_PIN_11,
	CHIP_7400_B4 = CHIP_PIN_12,
	CHIP_7400_A4 = CHIP_PIN_13
} Chip7400SignalAssignment;

#define CHIP_7400_PIN_COUNT 14
typedef Signal Chip7400Signals[CHIP_7400_PIN_COUNT];

typedef struct Chip7400Nand {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip7400Signals		signals;
} Chip7400Nand;

// types - 7474
typedef enum {
	CHIP_7474_CLR1_B = CHIP_PIN_01,
	CHIP_7474_D1	 = CHIP_PIN_02,
	CHIP_7474_CLK1   = CHIP_PIN_03,
	CHIP_7474_PR1_B  = CHIP_PIN_04,
	CHIP_7474_Q1	 = CHIP_PIN_05,
	CHIP_7474_Q1_B	 = CHIP_PIN_06,
	CHIP_7474_Q2_B	 = CHIP_PIN_08,
	CHIP_7474_Q2	 = CHIP_PIN_09,
	CHIP_7474_PR2_B  = CHIP_PIN_10,
	CHIP_7474_CLK2	 = CHIP_PIN_11,
	CHIP_7474_D2	 = CHIP_PIN_12,
	CHIP_7474_CLR2_B = CHIP_PIN_13
} Chip7474SignalAssignment;

#define CHIP_7474_PIN_COUNT 14
typedef Signal Chip7474Signals[CHIP_7474_PIN_COUNT];

typedef struct Chip7474DFlipFlop {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip7474Signals		signals;

	bool				q1;
	bool				q1_b;
	bool				q2;
	bool				q2_b;
} Chip7474DFlipFlop;

// types - 7493 (4-Bit Binary Counter)
typedef enum {
	CHIP_7493_B_B = CHIP_PIN_01,
	CHIP_7493_R01 = CHIP_PIN_02,
	CHIP_7493_R02 = CHIP_PIN_03,
	CHIP_7493_QC = CHIP_PIN_08,
	CHIP_7493_QB = CHIP_PIN_09,
	CHIP_7493_QD = CHIP_PIN_11,
	CHIP_7493_QA = CHIP_PIN_12,
	CHIP_7493_A_B = CHIP_PIN_14
} Chip7493SignalAssignment;

#define CHIP_7493_PIN_COUNT 14
typedef Signal Chip7493Signals[CHIP_7493_PIN_COUNT];

typedef struct Chip7493BinaryCounter {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip7493Signals		signals;

	bool				count_a;
	int					count_b;
} Chip7493BinaryCounter;

// types - 74107
typedef enum {
	CHIP_74107_J1 = CHIP_PIN_01,
	CHIP_74107_Q1_B = CHIP_PIN_02,
	CHIP_74107_Q1 = CHIP_PIN_03,
	CHIP_74107_K1 = CHIP_PIN_04,
	CHIP_74107_Q2 = CHIP_PIN_05,
	CHIP_74107_Q2_B = CHIP_PIN_06,
	CHIP_74107_J2 = CHIP_PIN_08,
	CHIP_74107_CLK2 = CHIP_PIN_09,
	CHIP_74107_CLR2_B = CHIP_PIN_10,
	CHIP_74107_K2 = CHIP_PIN_11,
	CHIP_74107_CLK1 = CHIP_PIN_12,
	CHIP_74107_CLR1_B = CHIP_PIN_13
} Chip74107SignalAssigment;

#define CHIP_74107_PIN_COUNT 14
typedef Signal Chip74107Signals[CHIP_74107_PIN_COUNT];

typedef struct Chip74107JKFlipFlop {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74107Signals	signals;

	bool				q1;
	bool				q2;
} Chip74107JKFlipFlop;

// types - 74145
typedef enum {
	CHIP_74145_Y0_B = CHIP_PIN_01,
	CHIP_74145_Y1_B = CHIP_PIN_02,
	CHIP_74145_Y2_B = CHIP_PIN_03,
	CHIP_74145_Y3_B = CHIP_PIN_04,
	CHIP_74145_Y4_B = CHIP_PIN_05,
	CHIP_74145_Y5_B = CHIP_PIN_06,
	CHIP_74145_Y6_B = CHIP_PIN_07,
	CHIP_74145_Y7_B = CHIP_PIN_09,
	CHIP_74145_Y8_B = CHIP_PIN_10,
	CHIP_74145_Y9_B = CHIP_PIN_11,
	CHIP_74145_D = CHIP_PIN_12,
	CHIP_74145_C = CHIP_PIN_13,
	CHIP_74145_B = CHIP_PIN_14,
	CHIP_74145_A = CHIP_PIN_15
} Chip74145SignalAssignment;

#define CHIP_74145_PIN_COUNT 16
typedef Signal Chip74145Signals[CHIP_74145_PIN_COUNT];

typedef struct Chip74145BcdDecoder {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74145Signals	signals;
} Chip74145BcdDecoder;

// types - 74153
typedef enum {
	CHIP_74153_G1 = CHIP_PIN_01,	// strobe group 1
	CHIP_74153_B = CHIP_PIN_02,		// select bit 2
	CHIP_74153_C13 = CHIP_PIN_03,	// data input group 1 bit 3
	CHIP_74153_C12 = CHIP_PIN_04,	// data input group 1 bit 2
	CHIP_74153_C11 = CHIP_PIN_05,	// data input group 1 bit 1
	CHIP_74153_C10 = CHIP_PIN_06,	// data input group 1 bit 0
	CHIP_74153_Y1 = CHIP_PIN_07,	// data output group 1
	CHIP_74153_Y2 = CHIP_PIN_09,	// data output group 1
	CHIP_74153_C20 = CHIP_PIN_10,	// data input group 1 bit 0
	CHIP_74153_C21 = CHIP_PIN_11,	// data input group 1 bit 1
	CHIP_74153_C22 = CHIP_PIN_12,	// data input group 1 bit 2
	CHIP_74153_C23 = CHIP_PIN_13,	// data input group 1 bit 3
	CHIP_74153_A = CHIP_PIN_14,		// select bit 2
	CHIP_74153_G2 = CHIP_PIN_15		// strobe group 2
} Chip74153SignalAssignment;

#define CHIP_74153_PIN_COUNT 16
typedef Signal Chip74153Signals[CHIP_74153_PIN_COUNT];

typedef struct Chip74153Multiplexer {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74153Signals	signals;

	Signal *			inputs[2][4];
} Chip74153Multiplexer;

// types - 74154
typedef enum {
	CHIP_74154_Y0_B = CHIP_PIN_01,
	CHIP_74154_Y1_B = CHIP_PIN_02,
	CHIP_74154_Y2_B = CHIP_PIN_03,
	CHIP_74154_Y3_B = CHIP_PIN_04,
	CHIP_74154_Y4_B = CHIP_PIN_05,
	CHIP_74154_Y5_B = CHIP_PIN_06,
	CHIP_74154_Y6_B = CHIP_PIN_07,
	CHIP_74154_Y7_B = CHIP_PIN_08,
	CHIP_74154_Y8_B = CHIP_PIN_09,
	CHIP_74154_Y9_B = CHIP_PIN_10,
	CHIP_74154_Y10_B = CHIP_PIN_11,
	CHIP_74154_Y11_B = CHIP_PIN_13,
	CHIP_74154_Y12_B = CHIP_PIN_14,
	CHIP_74154_Y13_B = CHIP_PIN_15,
	CHIP_74154_Y14_B = CHIP_PIN_16,
	CHIP_74154_Y15_B = CHIP_PIN_17,
	CHIP_74154_G1_B = CHIP_PIN_18,
	CHIP_74154_G2_B = CHIP_PIN_19,
	CHIP_74154_D = CHIP_PIN_20,
	CHIP_74154_C = CHIP_PIN_21,
	CHIP_74154_B = CHIP_PIN_22,
	CHIP_74154_A = CHIP_PIN_23
} Chip74154SignalAssignment;

#define CHIP_74154_PIN_COUNT 24
typedef Signal Chip74154Signals[CHIP_74154_PIN_COUNT];

typedef struct Chip74154Decoder {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74154Signals	signals;
} Chip74154Decoder;

// types - 74157 (Quad 2-Input Multiplexer)
typedef enum {
	CHIP_74157_SEL = CHIP_PIN_01,	// common select input
	CHIP_74157_I0A = CHIP_PIN_02,	// input 0 a
	CHIP_74157_I1A = CHIP_PIN_03,	// input 1 a
	CHIP_74157_ZA = CHIP_PIN_04,	// output a
	CHIP_74157_I0B = CHIP_PIN_05,	// input 0 b
	CHIP_74157_I1B = CHIP_PIN_06,	// input 1 b
	CHIP_74157_ZB = CHIP_PIN_07,	// output b
	CHIP_74157_ZD = CHIP_PIN_09,	// output d
	CHIP_74157_I1D = CHIP_PIN_10,	// input 1 d
	CHIP_74157_I0D = CHIP_PIN_11,	// input 0 d
	CHIP_74157_ZC = CHIP_PIN_12,	// output c
	CHIP_74157_I1C = CHIP_PIN_13,	// input 1 c
	CHIP_74157_I0C = CHIP_PIN_14,	// input 0 c
	CHIP_74157_ENABLE_B = CHIP_PIN_15,	// chip enable
} Chip74157SignalAssigment;

#define CHIP_74157_PIN_COUNT 16
typedef Signal Chip74157Signals[CHIP_74157_PIN_COUNT];

typedef struct Chip74157Multiplexer {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74157Signals	signals;
} Chip74157Multiplexer;

// types - 74164 (8-Bit Serial In/Parallel Out Shift Register)
typedef enum {
	CHIP_74164_A = CHIP_PIN_01,		// serial input A
	CHIP_74164_B = CHIP_PIN_02,		// serial input B
	CHIP_74164_QA = CHIP_PIN_03,	// parallel output A
	CHIP_74164_QB = CHIP_PIN_04,	// parallel output B
	CHIP_74164_QC = CHIP_PIN_05,	// parallel output C
	CHIP_74164_QD = CHIP_PIN_06,	// parallel output D
	CHIP_74164_CLK = CHIP_PIN_08,	// clock
	CHIP_74164_CLEAR_B = CHIP_PIN_09,	// reset
	CHIP_74164_QE = CHIP_PIN_10,	// parallel output A
	CHIP_74164_QF = CHIP_PIN_11,	// parallel output B
	CHIP_74164_QG = CHIP_PIN_12,	// parallel output C
	CHIP_74164_QH = CHIP_PIN_13,	// parallel output D
} Chip74164SignalAssignment;

#define CHIP_74164_PIN_COUNT 14
typedef Signal Chip74164Signals[CHIP_74164_PIN_COUNT];

typedef struct Chip74164ShiftRegister {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74164Signals	signals;

	uint8_t				state;
} Chip74164ShiftRegister;

// types - 74165 (8-Bit Parallel In/Serial Output Shift Registers)
typedef enum {
	CHIP_74165_SL = CHIP_PIN_01,	// shift or load
	CHIP_74165_CLK = CHIP_PIN_02,	// clock
	CHIP_74165_E = CHIP_PIN_03,		// parallel input
	CHIP_74165_F = CHIP_PIN_04,		// parallel input
	CHIP_74165_G = CHIP_PIN_05,		// parallel input
	CHIP_74165_H = CHIP_PIN_06,		// parallel input
	CHIP_74165_QH_B = CHIP_PIN_07,	// output /Qh
	CHIP_74165_QH = CHIP_PIN_09,	// output Qh
	CHIP_74165_SI = CHIP_PIN_10,	// serial input
	CHIP_74165_A = CHIP_PIN_11,		// parallel input
	CHIP_74165_B = CHIP_PIN_12,		// parallel input
	CHIP_74165_C = CHIP_PIN_13,		// parallel input
	CHIP_74165_D = CHIP_PIN_14,		// parallel input
	CHIP_74165_CLK_INH = CHIP_PIN_15,	// clock inhibit
} Chip74165SignalAssignment;

#define CHIP_74165_PIN_COUNT 16
typedef Signal Chip74165Signals[CHIP_74165_PIN_COUNT];

typedef struct Chip74165ShiftRegister {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74165Signals	signals;

	int					state;
	bool				prev_gated_clk;
} Chip74165ShiftRegister;

// types - 74177 (Presettable Binary Counter/Latch)
typedef enum {
	CHIP_74177_LOAD_B = CHIP_PIN_01,
	CHIP_74177_QC = CHIP_PIN_02,
	CHIP_74177_C = CHIP_PIN_03,
	CHIP_74177_A = CHIP_PIN_04,
	CHIP_74177_QA = CHIP_PIN_05,
	CHIP_74177_CLK2 = CHIP_PIN_06,
	CHIP_74177_CLK1 = CHIP_PIN_08,
	CHIP_74177_QB = CHIP_PIN_09,
	CHIP_74177_B = CHIP_PIN_10,
	CHIP_74177_D = CHIP_PIN_11,
	CHIP_74177_QD = CHIP_PIN_12,
	CHIP_74177_CLEAR_B = CHIP_PIN_13,
} Chip74177SignalAssignment;

#define CHIP_74177_PIN_COUNT 14
typedef Signal Chip74177Signals[CHIP_74177_PIN_COUNT];

typedef struct Chip74177BinaryCounter {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74177Signals	signals;

	bool				count_1;
	int					count_2;
} Chip74177BinaryCounter;

// types - 74191 (4-Bit Synchronous Up/Down Binary Counter)
typedef enum {
	CHIP_74191_B = CHIP_PIN_01,
	CHIP_74191_QB = CHIP_PIN_02,
	CHIP_74191_QA = CHIP_PIN_03,
	CHIP_74191_ENABLE_B = CHIP_PIN_04,
	CHIP_74191_D_U = CHIP_PIN_05,
	CHIP_74191_QC = CHIP_PIN_06,
	CHIP_74191_QD = CHIP_PIN_07,
	CHIP_74191_D = CHIP_PIN_09,
	CHIP_74191_C = CHIP_PIN_10,
	CHIP_74191_LOAD_B = CHIP_PIN_11,
	CHIP_74191_MAX_MIN = CHIP_PIN_12,
	CHIP_74191_RCO_B = CHIP_PIN_13,
	CHIP_74191_CLK = CHIP_PIN_14,
	CHIP_74191_A = CHIP_PIN_15
} Chip74191SignalAssignment;

#define CHIP_74191_PIN_COUNT 16
typedef Signal Chip74191Signals[CHIP_74191_PIN_COUNT];

typedef struct Chip74191BinaryCounter {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74191Signals	signals;

	int					state;
	bool				max_min;
} Chip74191BinaryCounter;

// types - 74244
typedef enum {
	CHIP_74244_G1_B = CHIP_PIN_01,	// enable output on group 1
	CHIP_74244_A11 = CHIP_PIN_02,	// group 1 input 1
	CHIP_74244_Y24 = CHIP_PIN_03,	// group 2 output 4
	CHIP_74244_A12 = CHIP_PIN_04,	// group 1 input 2
	CHIP_74244_Y23 = CHIP_PIN_05,	// group 2 output 3
	CHIP_74244_A13 = CHIP_PIN_06,	// group 1 input 3
	CHIP_74244_Y22 = CHIP_PIN_07,	// group 2 output 2
	CHIP_74244_A14 = CHIP_PIN_08,	// group 1 input 4
	CHIP_74244_Y21 = CHIP_PIN_09,	// group 2 output 1
	CHIP_74244_A21 = CHIP_PIN_11,	// group 2 input 1
	CHIP_74244_Y14 = CHIP_PIN_12,	// group 1 output 4
	CHIP_74244_A22 = CHIP_PIN_13,	// group 2 input 2
	CHIP_74244_Y13 = CHIP_PIN_14,	// group 1 output 3
	CHIP_74244_A23 = CHIP_PIN_15,	// group 2 input 3
	CHIP_74244_Y12 = CHIP_PIN_16,	// group 1 output 2
	CHIP_74244_A24 = CHIP_PIN_17,	// group 2 input 4
	CHIP_74244_Y11 = CHIP_PIN_18,	// group 1 output 1
	CHIP_74244_G2_B = CHIP_PIN_19,	// enable output on group 2
} Chip74244SignalAssignment;

#define CHIP_74244_PIN_COUNT 20
typedef Signal Chip74244Signals[CHIP_74244_PIN_COUNT];

typedef struct Chip74244OctalBuffer {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74244Signals	signals;
} Chip74244OctalBuffer;

// types - 74373 (Octal D-Type Transparant Latches)
typedef enum {
	CHIP_74373_OC_B = CHIP_PIN_01,	// output control
	CHIP_74373_Q1 = CHIP_PIN_02,
	CHIP_74373_D1 = CHIP_PIN_03,
	CHIP_74373_D2 = CHIP_PIN_04,
	CHIP_74373_Q2 = CHIP_PIN_05,
	CHIP_74373_Q3 = CHIP_PIN_06,
	CHIP_74373_D3 = CHIP_PIN_07,
	CHIP_74373_D4 = CHIP_PIN_08,
	CHIP_74373_Q4 = CHIP_PIN_09,
	CHIP_74373_C = CHIP_PIN_11,		// enable input
	CHIP_74373_Q5 = CHIP_PIN_12,
	CHIP_74373_D5 = CHIP_PIN_13,
	CHIP_74373_D6 = CHIP_PIN_14,
	CHIP_74373_Q6 = CHIP_PIN_15,
	CHIP_74373_Q7 = CHIP_PIN_16,
	CHIP_74373_D7 = CHIP_PIN_17,
	CHIP_74373_D8 = CHIP_PIN_18,
	CHIP_74373_Q8 = CHIP_PIN_19
} Chip74373SignalAssignment;

#define CHIP_74373_PIN_COUNT 20
typedef Signal Chip74373Signals[CHIP_74373_PIN_COUNT];

typedef struct Chip74373Latch {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Chip74373Signals	signals;

	uint8_t				state;
} Chip74373Latch;

// functions
Chip7400Nand *chip_7400_nand_create(struct Simulator *sim, Chip7400Signals signals);

Chip7474DFlipFlop *chip_7474_d_flipflop_create(struct Simulator *sim, Chip7474Signals signals);

Chip7493BinaryCounter *chip_7493_binary_counter_create(struct Simulator *sim, Chip7493Signals signals);

Chip74107JKFlipFlop *chip_74107_jk_flipflop_create(struct Simulator *sim, Chip74107Signals signals);

Chip74145BcdDecoder *chip_74145_bcd_decoder_create(struct Simulator *sim, Chip74145Signals signals);

Chip74153Multiplexer *chip_74153_multiplexer_create(struct Simulator *sim, Chip74153Signals signals);

Chip74154Decoder *chip_74154_decoder_create(struct Simulator *sim, Chip74154Signals signals);

Chip74157Multiplexer *chip_74157_multiplexer_create(struct Simulator *sim, Chip74157Signals signals);

Chip74164ShiftRegister *chip_74164_shift_register_create(struct Simulator *sim, Chip74164Signals signals);

Chip74165ShiftRegister *chip_74165_shift_register_create(struct Simulator *sim, Chip74165Signals signals);

Chip74177BinaryCounter *chip_74177_binary_counter_create(struct Simulator *sim, Chip74177Signals signals);

Chip74191BinaryCounter *chip_74191_binary_counter_create(struct Simulator *sim, Chip74191Signals signals);
Chip74244OctalBuffer *chip_74244_octal_buffer_create(struct Simulator *sim, Chip74244Signals signals);
Chip74373Latch *chip_74373_latch_create(struct Simulator *sim, Chip74373Signals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_74XXX_H
