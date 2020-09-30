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
typedef struct Cpu6502Signals {
	Signal 	bus_address;	// 16-bit address bus
	Signal	bus_data;		// 8-bit data bus
	Signal	clock;			// 1-bit clock-line (PHI2) - read-only
	Signal	reset_b;		// 1-bit reset-line (read-only)
	Signal	rw;				// 1-bit read-write line (true == reading, false == writing)
	Signal	irq_b;			// 1-bit interrupt request line
	Signal	nmi_b;			// 1-bit non-maskable interrupt line
	Signal  sync;			// 1-bit output - signals opcode fetch cycle
	Signal	rdy;			// 1-bit ready signal - cpu only runs when asserted
} Cpu6502Signals;

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
void cpu_6502_destroy(Cpu6502 *cpu);
void cpu_6502_register_dependencies(Cpu6502 *cpu);
void cpu_6502_process(Cpu6502 *cpu);

bool cpu_6502_in_initialization(Cpu6502 *cpu);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CPU_6502_H
