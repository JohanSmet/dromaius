// chip_6520.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the 6520 Peripheral Interface Adapter

#ifndef DROMAIUS_CHIP_6520_H
#define DROMAIUS_CHIP_6520_H

#include "types.h"
#include "signal_line.h"

// types
typedef union ctrl_reg_t {
	uint8_t		reg;
	struct {
		unsigned int bf_cl1_control : 2;
		unsigned int bf_ddr_or_select : 1;
		unsigned int bf_cl2_control : 3;
		unsigned int bf_irq2 : 1;
		unsigned int bf_irq1 : 1;
	};
} ctrl_reg_t;

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
	Signal		clock;			// 1-bit clock-line (PHI2) (In)
	Signal		cs0;			// 1-bit chip select 0 (In)
	Signal		cs1;			// 1-bit chip select 1 (In)
	Signal		cs2_b;			// 1-bit chip select 2 (In)
	Signal		rw;				// 1-bit read/write-line (true = read (pia -> cpu), false = write (cpu -> pia))
} Chip6520Signals;

typedef struct Chip6520 {
	// interface
	SignalPool *		signal_pool;
	Chip6520Signals		signals;

	// registers
	uint8_t		reg_ddra;		// 8-bit data direction register for port A
	ctrl_reg_t	reg_cra;		// 8-bit control register for port A
	uint8_t		reg_ora;		// 8-bit peripheral output register for port A

	uint8_t		reg_ddrb;		// 8-bit data direction register for port B
	ctrl_reg_t	reg_crb;		// 8-bit control register for port B
	uint8_t		reg_orb;		// 8-bit peripheral output register for port B

	uint8_t		reg_dir;		// 8-bit data input register

} Chip6520;

// functions
Chip6520 *chip_6520_create(SignalPool *signal_pool, Chip6520Signals signals);
void chip_6520_destroy(Chip6520 *pia);
void chip_6520_process(Chip6520 *pia);

#endif // DROMAIUS_CHIP_6520_H
