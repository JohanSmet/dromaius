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
typedef enum {
	// port-A (8-bit peripheral I/O port)
	CHIP_6520_PA0 = CHIP_PIN_02,
	CHIP_6520_PA1 = CHIP_PIN_03,
	CHIP_6520_PA2 = CHIP_PIN_04,
	CHIP_6520_PA3 = CHIP_PIN_05,
	CHIP_6520_PA4 = CHIP_PIN_06,
	CHIP_6520_PA5 = CHIP_PIN_07,
	CHIP_6520_PA6 = CHIP_PIN_08,
	CHIP_6520_PA7 = CHIP_PIN_09,

	// port-B (8-bit peripheral I/O port)
	CHIP_6520_PB0 = CHIP_PIN_10,
	CHIP_6520_PB1 = CHIP_PIN_11,
	CHIP_6520_PB2 = CHIP_PIN_12,
	CHIP_6520_PB3 = CHIP_PIN_13,
	CHIP_6520_PB4 = CHIP_PIN_14,
	CHIP_6520_PB5 = CHIP_PIN_15,
	CHIP_6520_PB6 = CHIP_PIN_16,
	CHIP_6520_PB7 = CHIP_PIN_17,

	// data bus (8-bit)
	CHIP_6520_D0 = CHIP_PIN_33,
	CHIP_6520_D1 = CHIP_PIN_32,
	CHIP_6520_D2 = CHIP_PIN_31,
	CHIP_6520_D3 = CHIP_PIN_30,
	CHIP_6520_D4 = CHIP_PIN_29,
	CHIP_6520_D5 = CHIP_PIN_28,
	CHIP_6520_D6 = CHIP_PIN_27,
	CHIP_6520_D7 = CHIP_PIN_26,

	CHIP_6520_CA1 = CHIP_PIN_40,		// 1-bit interrupt input control line for port A - (In)
	CHIP_6520_CA2 = CHIP_PIN_39,		// 1-bit interrupt input/peripheral control line for port A - (In & Out)
	CHIP_6520_CB1 = CHIP_PIN_18,		// 1-bit interrupt input control line for port B - (In)
	CHIP_6520_CB2 = CHIP_PIN_19,		// 1-bit interrupt input/peripheral control line for port B - (In & Out)

	CHIP_6520_IRQA_B = CHIP_PIN_38,		// 1-bit interrupt request line for port A (Out)
	CHIP_6520_IRQB_B = CHIP_PIN_37,		// 1-bit interrupt request line for port A (Out)

	CHIP_6520_RS0 = CHIP_PIN_36,		// 1-bit register select 0 (In)
	CHIP_6520_RS1 = CHIP_PIN_35,		// 1-bit register select 1 (In)
	CHIP_6520_RESET_B = CHIP_PIN_34,	// 1-bit reset-line (In)

	CHIP_6520_PHI2 = CHIP_PIN_25,		// 1-bit clock-line (In)

	CHIP_6520_CS0 = CHIP_PIN_22,		// 1-bit chip select 0 (In)
	CHIP_6520_CS1 = CHIP_PIN_24,		// 1-bit chip select 1 (In)
	CHIP_6520_CS2_B = CHIP_PIN_23,		// 1-bit chip select 2 (In)
	CHIP_6520_RW = CHIP_PIN_21,			// 1-bit read/write-line (true = read (pia -> cpu), false = write (cpu -> pia))
} Chip6520SignalAssignment;

typedef Signal Chip6520Signals[40];

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

	// signal groups
	SignalGroup			sg_port_a;
	SignalGroup			sg_port_b;
	SignalGroup			sg_data;

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
Chip6520 *chip_6520_create(struct Simulator *sim, Chip6520Signals signals);
void chip_6520_register_dependencies(Chip6520 *pia);
void chip_6520_destroy(Chip6520 *pia);
void chip_6520_process(Chip6520 *pia);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_6520_H
