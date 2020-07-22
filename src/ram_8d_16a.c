// ram_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a memory module with an 8-bit wide databus and a maximum of 16 datalines (64kb)

#include "ram_8d_16a.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			ram->signal_pool
#define SIGNAL_COLLECTION	ram->signals
#define SIGNAL_CHIP_ID		ram->id

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Ram8d16a *ram_8d16a_create(uint8_t num_address_lines, SignalPool *signal_pool, Ram8d16aSignals signals) {
	assert(num_address_lines > 0 && num_address_lines <= 16);

	size_t data_size = (size_t) 1 << num_address_lines;
	Ram8d16a *ram = (Ram8d16a *) malloc(sizeof(Ram8d16a) + data_size);

	memset(ram, 0, sizeof(Ram8d16a) + data_size);
	ram->signal_pool = signal_pool;
	ram->data_size = data_size;
	CHIP_SET_FUNCTIONS(ram, ram_8d16a_process, ram_8d16a_destroy, ram_8d16a_register_dependencies);

	memcpy(&ram->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(bus_address, num_address_lines);
	SIGNAL_DEFINE(bus_data, 8);
	SIGNAL_DEFINE(ce_b, 1);
	SIGNAL_DEFINE(we_b, 1);
	SIGNAL_DEFINE(oe_b, 1);

	return ram;
}

void ram_8d16a_register_dependencies(Ram8d16a *ram) {
	assert(ram);
	signal_add_dependency(ram->signal_pool, SIGNAL(bus_address), ram->id);
	signal_add_dependency(ram->signal_pool, SIGNAL(bus_data), ram->id);
	signal_add_dependency(ram->signal_pool, SIGNAL(ce_b), ram->id);
	signal_add_dependency(ram->signal_pool, SIGNAL(we_b), ram->id);
	signal_add_dependency(ram->signal_pool, SIGNAL(oe_b), ram->id);
}

void ram_8d16a_destroy(Ram8d16a *ram) {
	assert(ram);
	free(ram);
}

void ram_8d16a_process(Ram8d16a *ram) {
	assert(ram);

	if (!ACTLO_ASSERTED(SIGNAL_BOOL(ce_b))) {
		return;
	}

	uint16_t address = SIGNAL_UINT16(bus_address);

	if (ACTLO_ASSERTED(SIGNAL_BOOL(oe_b))) {
		SIGNAL_SET_UINT8(bus_data, ram->data_array[address]);
	} else if (ACTLO_ASSERTED(SIGNAL_BOOL(we_b))) {
		ram->data_array[address] = SIGNAL_UINT8(bus_data);
	}
}
