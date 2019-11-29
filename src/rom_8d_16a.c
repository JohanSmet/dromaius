// rom_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a read-only memory module with an 8-bit wide databus and a maximum of 16 datalines (e.g. a 27c512)

#include "rom_8d_16a.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Rom8d16a *rom_8d16a_create(uint8_t num_address_lines) {
	assert(num_address_lines > 0 && num_address_lines <= 16);

	size_t data_size = 1 << num_address_lines;

	Rom8d16a *rom = (Rom8d16a *) malloc(sizeof(Rom8d16a) + data_size);
	memset(rom, 0, sizeof(Rom8d16a) + data_size);

	rom->pin_ce_b = ACTLO_DEASSERT;
	rom->data_size = data_size;
	rom->msk_address = data_size - 1;
	
	return rom;
}

void rom_8d16a_destroy(Rom8d16a *rom) {
	assert(rom);
	free(rom);
}

void rom_8d16a_process(Rom8d16a *rom) {
	assert(rom);

	rom->upd_data = false;

	if (!ACTLO_ASSERTED(rom->pin_ce_b)) {
		return;
	}

	rom->bus_address = rom->bus_address & rom->msk_address;
	rom->bus_data = rom->data_array[rom->bus_address];
	rom->upd_data = true;
}
