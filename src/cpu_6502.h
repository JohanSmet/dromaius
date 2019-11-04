// cpu_6502.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the MOS 6502

#ifndef DROMAIUS_CPU_6502_H
#define DROMAIUS_CPU_6502_H

#include "types.h"

// types
typedef struct Cpu6502 {
	uint16_t *	bus_address;		// 16-bit address bus
	uint8_t	*	bus_data;			// 8-bit data bus
	const bool *pin_clock;			// 1-bit clock-line (PHI2) - read-only
	const bool *pin_reset;			// 1-bit reset-line (read-only)
	bool *		pin_rw;				// 1-bit read-write line
} Cpu6502;

// functions
Cpu6502 *cpu_6502_create(
		uint16_t *addres_bus,		// 16-bit address bus
		uint8_t *data_bus,			// 8-bit data bus
		const bool *clock,			// 1-bit clock-line (PHI2) - read-only
		const bool *reset,			// 1-bit reset-line (read-only)
		bool *rw					// 1-bit read-write line
);

void cpu_6502_process(Cpu6502 *cpu);


#endif // DROMAIUS_CPU_6502_H
