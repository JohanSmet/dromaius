// rom_8d_16a.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a read-only memory module with an 8-bit wide databus and a maximum of 16 datalines (e.g. a 27c512)

#ifndef DROMAIUS_ROM_8D_16A_H
#define DROMAIUS_ROM_8D_16A_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
enum Rom8d16aSignalAssignment {

	// 16-bit address bus
	CHIP_ROM8D16A_A0 = CHIP_PIN_01,
	CHIP_ROM8D16A_A1 = CHIP_PIN_02,
	CHIP_ROM8D16A_A2 = CHIP_PIN_03,
	CHIP_ROM8D16A_A3 = CHIP_PIN_04,
	CHIP_ROM8D16A_A4 = CHIP_PIN_05,
	CHIP_ROM8D16A_A5 = CHIP_PIN_06,
	CHIP_ROM8D16A_A6 = CHIP_PIN_07,
	CHIP_ROM8D16A_A7 = CHIP_PIN_08,
	CHIP_ROM8D16A_A8 = CHIP_PIN_09,
	CHIP_ROM8D16A_A9 = CHIP_PIN_10,
	CHIP_ROM8D16A_A10 = CHIP_PIN_11,
	CHIP_ROM8D16A_A11 = CHIP_PIN_12,
	CHIP_ROM8D16A_A12 = CHIP_PIN_13,
	CHIP_ROM8D16A_A13 = CHIP_PIN_14,
	CHIP_ROM8D16A_A14 = CHIP_PIN_15,
	CHIP_ROM8D16A_A15 = CHIP_PIN_16,

	// 8-bit data bus
	CHIP_ROM8D16A_D0 = CHIP_PIN_17,
	CHIP_ROM8D16A_D1 = CHIP_PIN_18,
	CHIP_ROM8D16A_D2 = CHIP_PIN_19,
	CHIP_ROM8D16A_D3 = CHIP_PIN_20,
	CHIP_ROM8D16A_D4 = CHIP_PIN_21,
	CHIP_ROM8D16A_D5 = CHIP_PIN_22,
	CHIP_ROM8D16A_D6 = CHIP_PIN_23,
	CHIP_ROM8D16A_D7 = CHIP_PIN_24,

	CHIP_ROM8D16A_CE_B = CHIP_PIN_25,				// 1-bit chip enable (active low)
};

#define CHIP_ROM8D16A_PIN_COUNT 25
typedef Signal Rom8d16aSignals[CHIP_ROM8D16A_PIN_COUNT];

typedef struct Rom8d16a {
	CHIP_DECLARE_BASE

	// interface
	SignalPool *		signal_pool;
	Rom8d16aSignals		signals;

	SignalGroup			sg_address;
	SignalGroup			sg_data;

	// data
	int64_t		output_delay;
	uint16_t	last_address;

	size_t		data_size;
	uint8_t		data_array[];
} Rom8d16a;


// functions
Rom8d16a *rom_8d16a_create(size_t num_address_lines, struct Simulator *sim, Rom8d16aSignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_ROM_8D_16A_H
