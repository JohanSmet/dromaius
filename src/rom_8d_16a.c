// rom_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a read-only memory module with an 8-bit wide databus and a maximum of 16 datalines (e.g. a 27c512)

#include "rom_8d_16a.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			rom->signal_pool
#define SIGNAL_COLLECTION	rom->signals

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Rom8d16a *rom_8d16a_create(size_t num_address_lines, SignalPool *signal_pool, Rom8d16aSignals signals) {

	size_t data_size = (size_t) 1 << num_address_lines;
	Rom8d16a *rom = (Rom8d16a *) malloc(sizeof(Rom8d16a) + data_size);

	memset(rom, 0, sizeof(Rom8d16a) + data_size);
	rom->signal_pool = signal_pool;
	rom->data_size = data_size;
	CHIP_SET_FUNCTIONS(rom, rom_8d16a_process, rom_8d16a_destroy, rom_8d16a_register_dependencies);

	memcpy(&rom->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(bus_address, 16);
	SIGNAL_DEFINE(bus_data, 8);
	SIGNAL_DEFINE(ce_b, 1);

	return rom;
}

void rom_8d16a_register_dependencies(Rom8d16a *rom) {
	assert(rom);
	signal_add_dependency(rom->signal_pool, SIGNAL(bus_address), rom->id);
	signal_add_dependency(rom->signal_pool, SIGNAL(ce_b), rom->id);
}

void rom_8d16a_destroy(Rom8d16a *rom) {
	assert(rom);
	free(rom);
}

void rom_8d16a_process(Rom8d16a *rom) {
	assert(rom);

	if (!ACTLO_ASSERTED(SIGNAL_BOOL(ce_b))) {
		return;
	}

	uint16_t address = SIGNAL_UINT16(bus_address);
	SIGNAL_SET_UINT8(bus_data, rom->data_array[address]);
}
