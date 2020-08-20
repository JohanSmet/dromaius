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

// types - 6316 2k 8bit rom
typedef struct Chip6316Signals {
	Signal a0;					// 10-bit address bus
	Signal a1;
	Signal a2;
	Signal a3;
	Signal a4;
	Signal a5;
	Signal a6;
	Signal a7;
	Signal a8;
	Signal a9;
	Signal a10;

	Signal d0;					// 8-bit address bus
	Signal d1;
	Signal d2;
	Signal d3;
	Signal d4;
	Signal d5;
	Signal d6;
	Signal d7;

	Signal cs1_b;				// 1-bit chip select active low
	Signal cs2_b;				// 1-bit chip select active low
	Signal cs3;					// 1-bit chip select active high
} Chip6316Signals;

#define ROM_6316_DATA_SIZE	2048

typedef struct Chip6316Rom {
	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	Chip6316Signals		signals;

	// data
	uint8_t				data_array[ROM_6316_DATA_SIZE];

} Chip6316Rom;


// functions
Chip6316Rom *chip_6316_rom_create(SignalPool *signal_pool, Chip6316Signals signals);
void chip_6316_rom_register_dependencies(Chip6316Rom *chip);
void chip_6316_rom_destroy(Chip6316Rom *chip);
void chip_6316_rom_process(Chip6316Rom *chip);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_ROM_H
