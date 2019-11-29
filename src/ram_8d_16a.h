// ram_8d_16a.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a memory module with an 8-bit wide databus and a maximum of 16 datalines (64kb)

#ifndef DROMAIUS_RAM_8D_16A_H
#define DROMAIUS_RAM_8D_16A_H

#include "types.h"

// types
typedef struct Ram8d16a {
	// interface
	uint16_t	bus_address;	// 16-bit address bus
	uint16_t	msk_address;	//	mask for the enabled address lines
	uint8_t		bus_data;		// 8-bit data bus
	bool		upd_data;		// bus_data was updated
	
	bool		pin_ce_b;		// 1-bit chip enable (active low)
	bool		pin_we_b;		// 1-bit write enable (acive low)
	bool		pin_oe_b;		// 1-bit output enable (active low)

	// data
	size_t		data_size;
	uint8_t		data_array[];

} Ram8d16a;

// functions
Ram8d16a *ram_8d16a_create(uint8_t num_address_lines);
void ram_8d16a_destroy(Ram8d16a *ram);
void ram_8d16a_process(Ram8d16a *ram);

#endif // DROMAIUS_RAM_8D_16A_H
