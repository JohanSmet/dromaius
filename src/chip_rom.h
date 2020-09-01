// chip_rom.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of read-only memory modules

#ifndef DROMAIUS_CHIP_ROM_H
#define DROMAIUS_CHIP_ROM_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CHIP_6332_A0 = CHIP_PIN_08,
	CHIP_6332_A1 = CHIP_PIN_07,
	CHIP_6332_A2 = CHIP_PIN_06,
	CHIP_6332_A3 = CHIP_PIN_05,
	CHIP_6332_A4 = CHIP_PIN_04,
	CHIP_6332_A5 = CHIP_PIN_03,
	CHIP_6332_A6 = CHIP_PIN_02,
	CHIP_6332_A7 = CHIP_PIN_01,
	CHIP_6332_A8 = CHIP_PIN_23,
	CHIP_6332_A9 = CHIP_PIN_22,
	CHIP_6332_A10 = CHIP_PIN_19,
	CHIP_6332_A11 = CHIP_PIN_18,

	CHIP_6332_D0 = CHIP_PIN_09,
	CHIP_6332_D1 = CHIP_PIN_10,
	CHIP_6332_D2 = CHIP_PIN_11,
	CHIP_6332_D3 = CHIP_PIN_13,
	CHIP_6332_D4 = CHIP_PIN_14,
	CHIP_6332_D5 = CHIP_PIN_15,
	CHIP_6332_D6 = CHIP_PIN_16,
	CHIP_6332_D7 = CHIP_PIN_17,

	CHIP_6332_CS1_B = CHIP_PIN_20,
	CHIP_6332_CS3 = CHIP_PIN_21,

	CHIP_6332_VSS = CHIP_PIN_12,
	CHIP_6332_VCC = CHIP_PIN_24
} Chip6332SignalAssignment;

typedef enum {
	CHIP_6316_A0 = CHIP_PIN_08,
	CHIP_6316_A1 = CHIP_PIN_07,
	CHIP_6316_A2 = CHIP_PIN_06,
	CHIP_6316_A3 = CHIP_PIN_05,
	CHIP_6316_A4 = CHIP_PIN_04,
	CHIP_6316_A5 = CHIP_PIN_03,
	CHIP_6316_A6 = CHIP_PIN_02,
	CHIP_6316_A7 = CHIP_PIN_01,
	CHIP_6316_A8 = CHIP_PIN_23,
	CHIP_6316_A9 = CHIP_PIN_22,
	CHIP_6316_A10 = CHIP_PIN_19,

	CHIP_6316_D0 = CHIP_PIN_09,
	CHIP_6316_D1 = CHIP_PIN_10,
	CHIP_6316_D2 = CHIP_PIN_11,
	CHIP_6316_D3 = CHIP_PIN_13,
	CHIP_6316_D4 = CHIP_PIN_14,
	CHIP_6316_D5 = CHIP_PIN_15,
	CHIP_6316_D6 = CHIP_PIN_16,
	CHIP_6316_D7 = CHIP_PIN_17,

	CHIP_6316_CS1_B = CHIP_PIN_20,
	CHIP_6316_CS2_B = CHIP_PIN_18,
	CHIP_6316_CS3 = CHIP_PIN_21,

	CHIP_6316_VSS = CHIP_PIN_12,
	CHIP_6316_VCC = CHIP_PIN_24
} Chip6316SignalAssignment;

#define ROM_6316_DATA_SIZE	2048
#define ROM_6332_DATA_SIZE	4096

typedef Signal Chip63xxSignals[24];

typedef struct Chip63xxRom {
	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	Chip63xxSignals		signals;

	// data
	int64_t		output_delay;
	int			last_address;

	size_t		data_size;
	uint8_t		data_array[];
} Chip63xxRom;

// functions
Chip63xxRom *chip_6316_rom_create(SignalPool *signal_pool, Chip63xxSignals signals);
Chip63xxRom *chip_6332_rom_create(SignalPool *signal_pool, Chip63xxSignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_ROM_H
