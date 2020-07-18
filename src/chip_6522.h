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
typedef struct Chip6522Signals {
	Signal		bus_data;		// 8-bit databus [D0 - D7] - (In & Out)
	Signal		port_a;			// 8-bit peripheral I/O port A [PA0 - PA7] - (In & Out)
	Signal		port_b;			// 8-bit peripheral I/O port B [PB0 - PB7] - (In & Out)
	Signal		ca1;			// 1-bit interrupt input control line for port A - (In)
	Signal		ca2;			// 1-bit interrupt input/peripheral control line for port A - (In & Out)
	Signal		cb1;			// 1-bit interrupt input control line for port B - (In)
	Signal		cb2;			// 1-bit interrupt input/peripheral control line for port B - (In & Out)
	Signal		irq_b;			// 1-bit interrupt request line (Out)
	Signal		rs0;			// 1-bit register select 0 (In)
	Signal		rs1;			// 1-bit register select 1 (In)
	Signal		rs2;			// 1-bit register select 2 (In)
	Signal		rs3;			// 1-bit register select 3 (In)
	Signal		reset_b;		// 1-bit reset-line (In)
	Signal		enable;			// 1-bit clock-line (PHI2) (In)
	Signal		cs1;			// 1-bit chip select 1 (In)
	Signal		cs2_b;			// 1-bit chip select 2 (In)
	Signal		rw;				// 1-bit read/write-line (true = read (via -> cpu), false = write (cpu -> via))
} Chip6522Signals;

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
Chip6522 *chip_6522_create(SignalPool *signal_pool, Chip6522Signals signals);
void chip_6522_register_dependencies(Chip6522 *via);
void chip_6522_destroy(Chip6522 *via);
void chip_6522_process(Chip6522 *via);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_6522_H
