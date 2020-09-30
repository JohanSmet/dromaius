// chip_ram_dynamic.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of various dynamic random access memory chips

#ifndef DROMAIUS_RAM_DYNAMIC_H
#define DROMAIUS_RAM_DYNAMIC_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types - 8x MK 4116 (16K x 1 Bit Dynamic Ram)
//	- 8 parallel chips in one type because I don't want to waste 8x the memory
//	- does not emulate refresh cycle (or lack thereof)
typedef struct Chip8x4116DRamSignals {
	Signal	bus_address;		// 7-bit address bus
	Signal	we_b;				// 1-bit write enable (active low)
	Signal	ras_b;				// 1-bit row-address-select (active low)
	Signal	cas_b;				// 1-bit column-address-select (active low)

	Signal	bus_di;				// 8-bit
	Signal	bus_do;				// 8-bit
} Chip8x4116DRamSignals;

typedef struct Chip8x4116DRam {

	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	Chip8x4116DRamSignals	signals;

	// config
	int64_t		access_time;

	// data
	uint8_t		row;
	uint8_t		col;
	uint8_t		do_latch;
	uint8_t		data_array[128 * 128];

	int			state;
	int64_t		next_state_transition;
} Chip8x4116DRam;

// functions - 8x MK 4116
Chip8x4116DRam *chip_8x4116_dram_create(struct Simulator *simulator, Chip8x4116DRamSignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_RAM_DYNAMIC_H
