// chip_6520.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the 6520 Peripheral Interface Adapter

#ifndef DROMAIUS_CHIP_6520_H
#define DROMAIUS_CHIP_6520_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct Chip6520Signals {
	Signal		bus_data;		// 8-bit data bus [D0 - D7] - (In & Out)
	Signal		port_a;			// 8-bit peripheral I/O port A [PA0 - PA7] - (In & Out)
	Signal		port_b;			// 8-bit peripheral I/O port B [PB0 - PB7] - (In & Out)
	Signal		ca1;			// 1-bit interrupt input control line for port A - (In)
	Signal		ca2;			// 1-bit interrupt input/peripheral control line for port A - (In & Out)
	Signal		cb1;			// 1-bit interrupt input control line for port B - (In)
	Signal		cb2;			// 1-bit interrupt input/peripheral control line for port B - (In & Out)
	Signal		irqa_b;			// 1-bit interrupt request line for port A (Out)
	Signal		irqb_b;			// 1-bit interrupt request line for port A (Out)
	Signal		rs0;			// 1-bit register select 0 (In)
	Signal		rs1;			// 1-bit register select 1 (In)
	Signal		reset_b;		// 1-bit reset-line (In)
	Signal		enable;			// 1-bit clock-line (PHI2) (In)
	Signal		cs0;			// 1-bit chip select 0 (In)
	Signal		cs1;			// 1-bit chip select 1 (In)
	Signal		cs2_b;			// 1-bit chip select 2 (In)
	Signal		rw;				// 1-bit read/write-line (true = read (pia -> cpu), false = write (cpu -> pia))
} Chip6520Signals;

typedef enum Chip6520ControlFlags {
	// flags for cl2 == input (and common fields for cl2 == output)
	FLAG_6520_IRQ1_ENABLE			= 0b00000001,
	FLAG_6520_IRQ1_POS_TRANSITION	= 0b00000010,
	FLAG_6520_DDR_OR_SELECT			= 0b00000100,
	FLAG_6520_IRQ2_ENABLE			= 0b00001000,
	FLAG_6520_IRQ2_POS_TRANSITION	= 0b00010000,
	FLAG_6520_CL2_MODE_SELECT		= 0b00100000,
	FLAG_6520_IRQ2					= 0b01000000,
	FLAG_6520_IRQ1					= 0b10000000,

	// unique flags for cl2 == output
	FLAG_6520_CL2_RESTORE			= 0b00001000,
	FLAG_6520_CL2_OUTPUT			= 0b00010000
} Chip6520ControlFlags;

typedef struct Chip6520 {
	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	Chip6520Signals		signals;

	// registers
	uint8_t		reg_ddra;		// 8-bit data direction register for port A
	uint8_t		reg_cra;		// 8-bit control register for port A
	uint8_t		reg_ora;		// 8-bit peripheral output register for port A

	uint8_t		reg_ddrb;		// 8-bit data direction register for port B
	uint8_t		reg_crb;		// 8-bit control register for port B
	uint8_t		reg_orb;		// 8-bit peripheral output register for port B

	uint8_t		reg_dir;		// 8-bit data input register

} Chip6520;

// functions
Chip6520 *chip_6520_create(SignalPool *signal_pool, Chip6520Signals signals);
void chip_6520_destroy(Chip6520 *pia);
void chip_6520_process(Chip6520 *pia);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_6520_H
