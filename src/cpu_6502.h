// cpu_6502.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the MOS 6502

#ifndef DROMAIUS_CPU_6502_H
#define DROMAIUS_CPU_6502_H

#include "cpu.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
enum Cpu6502SignalAssignments {
	PIN_6502_RDY   = CHIP_PIN_02,		// ready signal - cpu only runs when asserted
	PIN_6502_PHI1O = CHIP_PIN_03,		// phase-1 clock output (NOT IMPLEMENTED)
	PIN_6502_IRQ_B = CHIP_PIN_04,		// interrupt request line
	PIN_6502_NMI_B = CHIP_PIN_06,		// non-maskable interrupt line
	PIN_6502_SYNC  = CHIP_PIN_07,		// indicates opcode fetch cycle (output)
	PIN_6502_AB0   = CHIP_PIN_09,
	PIN_6502_AB1   = CHIP_PIN_10,
	PIN_6502_AB2   = CHIP_PIN_11,
	PIN_6502_AB3   = CHIP_PIN_12,
	PIN_6502_AB4   = CHIP_PIN_13,
	PIN_6502_AB5   = CHIP_PIN_14,
	PIN_6502_AB6   = CHIP_PIN_15,
	PIN_6502_AB7   = CHIP_PIN_16,
	PIN_6502_AB8   = CHIP_PIN_17,
	PIN_6502_AB9   = CHIP_PIN_18,
	PIN_6502_AB10  = CHIP_PIN_19,
	PIN_6502_AB11  = CHIP_PIN_20,
	PIN_6502_AB12  = CHIP_PIN_22,
	PIN_6502_AB13  = CHIP_PIN_23,
	PIN_6502_AB14  = CHIP_PIN_24,
	PIN_6502_AB15  = CHIP_PIN_25,
	PIN_6502_DB7   = CHIP_PIN_26,
	PIN_6502_DB6   = CHIP_PIN_27,
	PIN_6502_DB5   = CHIP_PIN_28,
	PIN_6502_DB4   = CHIP_PIN_29,
	PIN_6502_DB3   = CHIP_PIN_30,
	PIN_6502_DB2   = CHIP_PIN_31,
	PIN_6502_DB1   = CHIP_PIN_32,
	PIN_6502_DB0   = CHIP_PIN_33,
	PIN_6502_RW	   = CHIP_PIN_34,		// read-write line (high == reading, low == writing)
	PIN_6502_CLK   = CHIP_PIN_37,		// phase-2 clock/system clock input
	PIN_6502_SO	   = CHIP_PIN_38,		// set overflow flag (NOT IMPLEMENTED)
	PIN_6502_PHI2O = CHIP_PIN_39,		// phase-1 clock output (NOT IMPLEMENTED)
	PIN_6502_RES_B = CHIP_PIN_40		// reset line (input)
};

#define CHIP_6502_PIN_COUNT 40
typedef Signal Cpu6502Signals[CHIP_6502_PIN_COUNT];

typedef enum Cpu6502Flags {
	FLAG_6502_CARRY				= 0b00000001,
	FLAG_6502_ZERO_RESULT		= 0b00000010,
	FLAG_6502_INTERRUPT_DISABLE = 0b00000100,
	FLAG_6502_DECIMAL_MODE		= 0b00001000,
	FLAG_6502_BREAK_COMMAND		= 0b00010000,
	FLAG_6502_EXPANSION			= 0b00100000,
	FLAG_6502_OVERFLOW			= 0b01000000,
	FLAG_6502_NEGATIVE_RESULT	= 0b10000000,

	// abbreviations
	FLAG_6502_C = FLAG_6502_CARRY,
	FLAG_6502_Z = FLAG_6502_ZERO_RESULT,
	FLAG_6502_I = FLAG_6502_INTERRUPT_DISABLE,
	FLAG_6502_D = FLAG_6502_DECIMAL_MODE,
	FLAG_6502_B = FLAG_6502_BREAK_COMMAND,
	FLAG_6502_E = FLAG_6502_EXPANSION,
	FLAG_6502_V = FLAG_6502_OVERFLOW,
	FLAG_6502_N = FLAG_6502_NEGATIVE_RESULT
} Cpu6502Flags;

typedef struct Cpu6502 {

	CPU_DECLARE_FUNCTIONS

	// interface
	SignalPool *	signal_pool;
	Cpu6502Signals	signals;

	SignalGroup		sg_address;
	SignalGroup		sg_data;

	// registers
	uint8_t		reg_a;				// accumulator
	uint8_t		reg_x;				// x-index
	uint8_t		reg_y;				// y-index
	uint8_t		reg_sp;				// stack-pointer
	uint8_t		reg_ir;				// instruction register
	uint16_t	reg_pc;				// program counter
	uint8_t		reg_p;				// processor status register
} Cpu6502;

// functions
Cpu6502 *cpu_6502_create(struct Simulator *sim, Cpu6502Signals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CPU_6502_H
