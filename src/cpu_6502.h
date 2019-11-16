// cpu_6502.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the MOS 6502

#ifndef DROMAIUS_CPU_6502_H
#define DROMAIUS_CPU_6502_H

#include "types.h"

// types
typedef struct Cpu6502 {
	// interface
	uint16_t *	bus_address;		// 16-bit address bus
	uint8_t	*	bus_data;			// 8-bit data bus
	const bool *pin_clock;			// 1-bit clock-line (PHI2) - read-only
	const bool *pin_reset_b;		// 1-bit reset-line (read-only)
	bool *		pin_rw;				// 1-bit read-write line (true == reading, false == writing)
	const bool *pin_irq_b;			// 1-bit interrupt request line
	const bool *pin_nmi_b;			// 1-bit non-maskable interrupt line

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
Cpu6502 *cpu_6502_create(
		uint16_t *addres_bus,		// 16-bit address bus
		uint8_t *data_bus,			// 8-bit data bus
		const bool *clock,			// 1-bit clock-line (PHI2) - read-only
		const bool *reset,			// 1-bit reset-line (read-only)
		bool *rw,					// 1-bit read-write line
		const bool *irq,			// 1-bit interrupt request line
		const bool *nmi				// 1-bit non-maskable interrupt line
);

void cpu_6502_process(Cpu6502 *cpu);


#endif // DROMAIUS_CPU_6502_H
