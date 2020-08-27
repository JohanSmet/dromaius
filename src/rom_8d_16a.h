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
typedef struct Rom8d16aSignals {
	struct Signal bus_address;		// 16-bit address bus
	struct Signal bus_data;			// 8-bit data bus
	struct Signal ce_b;				// 1-bit chip enable (active low)
} Rom8d16aSignals;


typedef struct Rom8d16a {
	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	Rom8d16aSignals		signals;

	// data
	int64_t		output_delay;
	uint16_t	last_address;

	size_t		data_size;
	uint8_t		data_array[];
} Rom8d16a;


// functions
Rom8d16a *rom_8d16a_create(size_t num_address_lines, SignalPool *signal_pool, Rom8d16aSignals signals);
void rom_8d16a_register_dependencies(Rom8d16a *rom);
void rom_8d16a_destroy(Rom8d16a *rom);
void rom_8d16a_process(Rom8d16a *rom);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_ROM_8D_16A_H
