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
typedef struct Chip6114SRamSignals {
	Signal	bus_address;		// 10-bit address bus
	Signal	bus_io;				// 4-bit data bus
	Signal	ce_b;				// 1-bit chip enable (active low)
	Signal	rw;					// 1-bit R/w select (high = read / low = write)
} Chip6114SRamSignals;

typedef struct Chip6114SRam {

	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	Chip6114SRamSignals	signals;

	// data
	uint8_t		data_array[1024];
} Chip6114SRam;

// functions - UM6114
Chip6114SRam *chip_6114_sram_create(struct Simulator *sim, Chip6114SRamSignals signals);
void chip_6114_sram_register_dependencies(Chip6114SRam *chip);
void chip_6114_sram_destroy(Chip6114SRam *chip);
void chip_6114_sram_process(Chip6114SRam *chip);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_RAM_STATIC_H
