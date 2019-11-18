// rom_8d_16a.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a read-only memory module with an 8-bit wide databus and a maximum of 16 datalines (e.g. a 27c512)

#ifndef DROMAIUS_ROM_8D_16A_H
#define DROMAIUS_ROM_8D_16A_H

#include "types.h"

// types
typedef struct Rom8d16a {
	// interface
	uint16_t	bus_address;	// 16-bit address bus
	uint16_t	msk_address;	//	mask for the enabled address lines
	uint8_t		bus_data;		// 8-bit data bus
	bool		upd_data;		// bus_data was updated
	
	bool		pin_ce_b;		// 1-bit chip enable (active low)

	// data
	uint8_t		data_array[];

} Rom8d16a;


// functions
Rom8d16a *rom_8d16a_create(uint8_t num_address_lines);
void rom_8d16a_destroy(Rom8d16a *rom);
void rom_8d16a_process(Rom8d16a *rom);

#endif // DROMAIUS_ROM_8D_16A_H
