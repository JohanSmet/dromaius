// chip_6522 - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the 6522 Versatile Interface Adapter

#ifndef DROMAIUS_CHIP_6522_H
#define DROMAIUS_CHIP_6522_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef enum {
	// port-A (8-bit peripheral I/O port)
	CHIP_6522_PA0 = CHIP_PIN_02,
	CHIP_6522_PA1 = CHIP_PIN_03,
	CHIP_6522_PA2 = CHIP_PIN_04,
	CHIP_6522_PA3 = CHIP_PIN_05,
	CHIP_6522_PA4 = CHIP_PIN_06,
	CHIP_6522_PA5 = CHIP_PIN_07,
	CHIP_6522_PA6 = CHIP_PIN_08,
	CHIP_6522_PA7 = CHIP_PIN_09,

	// port-B (8-bit peripheral I/O port)
	CHIP_6522_PB0 = CHIP_PIN_10,
	CHIP_6522_PB1 = CHIP_PIN_11,
	CHIP_6522_PB2 = CHIP_PIN_12,
	CHIP_6522_PB3 = CHIP_PIN_13,
	CHIP_6522_PB4 = CHIP_PIN_14,
	CHIP_6522_PB5 = CHIP_PIN_15,
	CHIP_6522_PB6 = CHIP_PIN_16,
	CHIP_6522_PB7 = CHIP_PIN_17,

	// data bus (8-bit)
	CHIP_6522_D0 = CHIP_PIN_33,
	CHIP_6522_D1 = CHIP_PIN_32,
	CHIP_6522_D2 = CHIP_PIN_31,
	CHIP_6522_D3 = CHIP_PIN_30,
	CHIP_6522_D4 = CHIP_PIN_29,
	CHIP_6522_D5 = CHIP_PIN_28,
	CHIP_6522_D6 = CHIP_PIN_27,
	CHIP_6522_D7 = CHIP_PIN_26,

	CHIP_6522_CA1 = CHIP_PIN_40,		// 1-bit interrupt input control line for port A - (In)
	CHIP_6522_CA2 = CHIP_PIN_39,		// 1-bit interrupt input/peripheral control line for port A - (In & Out)
	CHIP_6522_CB1 = CHIP_PIN_18,		// 1-bit interrupt input control line for port B - (In)
	CHIP_6522_CB2 = CHIP_PIN_19,		// 1-bit interrupt input/peripheral control line for port B - (In & Out)

	CHIP_6522_RS0 = CHIP_PIN_38,		// 1-bit register select 0 (In)
	CHIP_6522_RS1 = CHIP_PIN_37,		// 1-bit register select 1 (In)
	CHIP_6522_RS2 = CHIP_PIN_36,		// 1-bit register select 2 (In)
	CHIP_6522_RS3 = CHIP_PIN_35,		// 1-bit register select 3 (In)
	CHIP_6522_RESET_B = CHIP_PIN_34,	// 1-bit reset-line (In)

	CHIP_6522_PHI2 = CHIP_PIN_25,		// 1-bit clock-line (In)
	CHIP_6522_CS1 = CHIP_PIN_24,		// 1-bit chip select 1 (In)
	CHIP_6522_CS2_B = CHIP_PIN_23,		// 1-bit chip select 2 (In)
	CHIP_6522_RW = CHIP_PIN_22,			// 1-bit read/write-line (true = read (pia -> cpu), false = write (cpu -> pia))
	CHIP_6522_IRQ_B = CHIP_PIN_21		// 1-bit interrupt request line (Out)

} Chip6522SignalAssignment;

typedef Signal Chip6522Signals[40];

typedef enum Chip6522AuxiliaryControl {
	MASK_6522_ACR_PA_LATCH			= 0b00000001,
	MASK_6522_ACR_PB_LATCH			= 0b00000010,
	MASK_6522_ACR_SR_CONTROL		= 0b00011100,
	MASK_6522_ACR_T2_CONTROL		= 0b00100000,
	MASK_6522_ACR_T1_CONTROL		= 0b11000000,

	VALUE_6522_ACR_PA_NO_LATCH		= 0b00000000,
	VALUE_6522_ACR_PA_LATCH			= 0b00000001,

	VALUE_6522_ACR_PB_NO_LATCH		= 0b00000000,
	VALUE_6522_ACR_PB_LATCH			= 0b00000010,

	VALUE_6522_ACR_SR_DISABLED		= 0b00000000,
	VALUE_6522_ACR_SR_IN_T2			= 0b00000100,
	VALUE_6522_ACR_SR_IN_PHI2		= 0b00001000,
	VALUE_6522_ACR_SR_IN_EXT		= 0b00001100,
	VALUE_6522_ACR_SR_OUT_FREE_T2	= 0b00010000,
	VALUE_6522_ACR_SR_OUT_T2		= 0b00010100,
	VALUE_6522_ACR_SR_OUT_PHI2		= 0b00011000,
	VALUE_6522_ACR_SR_OUT_EXT		= 0b00011100,

	VALUE_6522_ACR_T2_TIMED			= 0b00000000,
	VALUE_6522_ACR_T2_PB6			= 0b00100000,

	VALUE_6522_ACR_T1_LOAD_NOPB7	= 0b00000000,
	VALUE_6522_ACR_T1_CONT_NOPB7	= 0b01000000,
	VALUE_6522_ACR_T1_LOAD_1SHOT 	= 0b10000000,
	VALUE_6522_ACR_T1_CONT_SQWAVE	= 0b11000000
} Chip6522AuxiliaryControl;

typedef enum Chip6552PeripheralControl {
	MASK_6522_PCR_CA1_CONTROL			= 0b00000001,
	MASK_6522_PCR_CA2_CONTROL			= 0b00001110,
	MASK_6522_PCR_CB1_CONTROL			= 0b00010000,
	MASK_6522_PCR_CB2_CONTROL			= 0b11100000,

	VALUE_6522_PCR_CA1_NEG_ACTIVE		= 0b00000000,
	VALUE_6522_PCR_CA1_POS_ACTIVE		= 0b00000001,

	VALUE_6522_PCR_CA2_IN_NEG			= 0b00000000,
	VALUE_6522_PCR_CA2_IN_NEG_INDEP		= 0b00000010,
	VALUE_6522_PCR_CA2_IN_POS			= 0b00000100,
	VALUE_6522_PCR_CA2_IN_POS_INDEP		= 0b00000110,
	VALUE_6522_PCR_CA2_OUT_HANDSHAKE  	= 0b00001000,
	VALUE_6522_PCR_CA2_OUT_PULSE	 	= 0b00001010,
	VALUE_6522_PCR_CA2_OUT_LOW		 	= 0b00001100,
	VALUE_6522_PCR_CA2_OUT_HIGH			= 0b00001110,

	VALUE_6522_PCR_CB1_NEG_ACTIVE		= 0b00000000,
	VALUE_6522_PCR_CB1_POS_ACTIVE		= 0b00010000,

	VALUE_6522_PCR_CB2_IN_NEG			= 0b00000000,
	VALUE_6522_PCR_CB2_IN_NEG_INDEP		= 0b00100000,
	VALUE_6522_PCR_CB2_IN_POS			= 0b01000000,
	VALUE_6522_PCR_CB2_IN_POS_INDEP	    = 0b01100000,
	VALUE_6522_PCR_CB2_OUT_HANDSHAKE  	= 0b10000000,
	VALUE_6522_PCR_CB2_OUT_PULSE	 	= 0b10100000,
	VALUE_6522_PCR_CB2_OUT_LOW			= 0b11000000,
	VALUE_6522_PCR_CB2_OUT_HIGH			= 0b11100000
} Chip6552PeripheralControl;

typedef enum Chip6522InterruptFlag {
	FLAG_6522_IFR_CA2	= 0b00000001,
	FLAG_6522_IFR_CA1	= 0b00000010,
	FLAG_6522_IFR_SR 	= 0b00000100,
	FLAG_6522_IFR_CB2	= 0b00001000,
	FLAG_6522_IFR_CB1	= 0b00010000,
	FLAG_6522_IFR_T2	= 0b00100000,
	FLAG_6522_IFR_T1	= 0b01000000,
	FLAG_6522_IFR_IRQ	= 0b10000000
} Chip6522InterruptFlag;

typedef enum Chip6522InterruptEnable {
	FLAG_6522_IER_CA2	= 0b00000001,
	FLAG_6522_IER_CA1	= 0b00000010,
	FLAG_6522_IER_SR 	= 0b00000100,
	FLAG_6522_IER_CB2	= 0b00001000,
	FLAG_6522_IER_CB1	= 0b00010000,
	FLAG_6522_IER_T2	= 0b00100000,
	FLAG_6522_IER_T1	= 0b01000000,
	FLAG_6522_IER_IRQ	= 0b10000000
} Chip6522InterruptEnable;

typedef struct Chip6522 {
	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	Chip6522Signals		signals;

	// signal groups
	SignalGroup			sg_port_a;
	SignalGroup			sg_port_b;
	SignalGroup			sg_data;

	// registers
	uint8_t		reg_ifr;		// interrupt control - flags
	uint8_t		reg_ier;		// interrupt control - enable
	uint8_t		reg_pcr;		// function control - peripheral
	uint8_t		reg_acr;		// function control - auxiliary
	uint8_t		reg_t1l_l;		// timer 1 - latch (low byte)
	uint8_t		reg_t1l_h;		// timer 1 - latch (high byte)
	uint16_t	reg_t1c;		// timer 1 - counter
	uint8_t		reg_t2l_l;		// timer 2 - latch
	uint16_t	reg_t2c;		// timer 2 - counter
	uint8_t		reg_ila;		// port-A - input latch
	uint8_t		reg_ora;		// port-A - output
	uint8_t		reg_ddra;		// port-A - data directions
	uint8_t		reg_ilb;		// port-A - input latch
	uint8_t		reg_orb;		// port-A - output
	uint8_t		reg_ddrb;		// port-A - data directions
	uint8_t		reg_sr;			// shift register
} Chip6522;


// functions
Chip6522 *chip_6522_create(struct Simulator *sim, Chip6522Signals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_6522_H
