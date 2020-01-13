// cpu_6502.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the MOS 6502

#ifndef DROMAIUS_CPU_6502_H
#define DROMAIUS_CPU_6502_H

#include "types.h"
#include "signal.h"

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

typedef struct Cpu6502 {
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
	union {
		uint8_t		reg_p;				// processor status register
		struct {
			unsigned int p_carry : 1;
			unsigned int p_zero_result : 1;
			unsigned int p_interrupt_disable : 1;
			unsigned int p_decimal_mode : 1;
			unsigned int p_break_command : 1;
			unsigned int p_expension : 1;
			unsigned int p_overflow : 1;
			unsigned int p_negative_result : 1;
		};
	};
} Cpu6502;

// functions
Cpu6502 *cpu_6502_create(SignalPool *signal_pool, Cpu6502Signals signals);
void cpu_6502_destroy(Cpu6502 *cpu);
void cpu_6502_process(Cpu6502 *cpu, bool delayed);

void cpu_6502_override_next_instruction_address(Cpu6502 *cpu, uint16_t pc);
bool cpu_6502_at_start_of_instruction(Cpu6502 *cpu);

#endif // DROMAIUS_CPU_6502_H
