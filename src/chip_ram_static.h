// chip_ram_static.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of various static random access memory chips

#ifndef DROMAIUS_RAM_STATIC_H
#define DROMAIUS_RAM_STATIC_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types - UM6114 (1Kx4 CMOS SRAM)
typedef enum {
	// 10-bit address bus
	CHIP_6114_A0 = CHIP_PIN_05,
	CHIP_6114_A1 = CHIP_PIN_06,
	CHIP_6114_A2 = CHIP_PIN_07,
	CHIP_6114_A3 = CHIP_PIN_04,
	CHIP_6114_A4 = CHIP_PIN_03,
	CHIP_6114_A5 = CHIP_PIN_02,
	CHIP_6114_A6 = CHIP_PIN_01,
	CHIP_6114_A7 = CHIP_PIN_17,
	CHIP_6114_A8 = CHIP_PIN_16,
	CHIP_6114_A9 = CHIP_PIN_15,

	// 4-bit data bus
	CHIP_6114_IO0 = CHIP_PIN_14,
	CHIP_6114_IO1 = CHIP_PIN_13,
	CHIP_6114_IO2 = CHIP_PIN_12,
	CHIP_6114_IO3 = CHIP_PIN_11,

	CHIP_6114_CE_B = CHIP_PIN_08,				// 1-bit chip enable (active low)
	CHIP_6114_RW = CHIP_PIN_10					// 1-bit R/w select (high = read / low = write)
} Chip6114SRamSignalAssignment;

typedef Signal Chip6114SRamSignals[18];

typedef struct Chip6114SRam {

	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	Chip6114SRamSignals	signals;

	SignalGroup			sg_address;
	SignalGroup			sg_io;

	// data
	uint8_t		data_array[1024];
} Chip6114SRam;

// functions - UM6114
Chip6114SRam *chip_6114_sram_create(struct Simulator *sim, Chip6114SRamSignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_RAM_STATIC_H
