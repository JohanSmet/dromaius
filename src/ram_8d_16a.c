// ram_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a memory module with an 8-bit wide databus and a maximum of 16 datalines (64kb)

#include "ram_8d_16a.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Ram8d16a *ram_8d16a_create(uint8_t num_address_lines) {
	assert(num_address_lines > 0 && num_address_lines <= 16);
	
	size_t data_size = 1 << num_address_lines;

	Ram8d16a *ram = (Ram8d16a *) malloc(sizeof(Ram8d16a) + data_size);
	memset(ram, 0, sizeof(Ram8d16a) + data_size);

	ram->pin_ce_b = ACTLO_DEASSERT;
	ram->pin_we_b = ACTLO_DEASSERT;
	ram->pin_oe_b = ACTLO_DEASSERT;

	ram->msk_address = data_size - 1;
	ram->data_size = data_size;

	return ram;
}

void ram_8d16a_destroy(Ram8d16a *ram) {
	assert(ram);
	free(ram);
}

void ram_8d16a_process(Ram8d16a *ram) {
	assert(ram);

	ram->upd_data = false;

	if (!ACTLO_ASSERTED(ram->pin_ce_b)) {
		return;
	}

	ram->bus_address = ram->bus_address & ram->msk_address;

	if (ACTLO_ASSERTED(ram->pin_oe_b)) {
		ram->bus_data = ram->data_array[ram->bus_address];
		ram->upd_data = true;
	} else if (ACTLO_ASSERTED(ram->pin_we_b)) {
		ram->data_array[ram->bus_address] = ram->bus_data;
	}
}
