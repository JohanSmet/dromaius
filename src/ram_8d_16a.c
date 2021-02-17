// ram_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a memory module with an 8-bit wide databus and a maximum of 16 datalines (64kb)

#include "ram_8d_16a.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_PREFIX		CHIP_RAM8D16A_
#define SIGNAL_OWNER		ram

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void ram_8d16a_register_dependencies(Ram8d16a *ram);
static void ram_8d16a_destroy(Ram8d16a *ram);
static void ram_8d16a_process(Ram8d16a *ram);

Ram8d16a *ram_8d16a_create(uint8_t num_address_lines, Simulator *sim, Ram8d16aSignals signals) {
	assert(num_address_lines > 0 && num_address_lines <= 16);

	size_t data_size = (size_t) 1 << num_address_lines;
	Ram8d16a *ram = (Ram8d16a *) calloc(1, sizeof(Ram8d16a) + data_size);

	ram->simulator = sim;
	ram->signal_pool = sim->signal_pool;
	ram->data_size = data_size;
	CHIP_SET_FUNCTIONS(ram, ram_8d16a_process, ram_8d16a_destroy, ram_8d16a_register_dependencies);

	memcpy(ram->signals, signals, sizeof(Ram8d16aSignals));

	ram->sg_address = signal_group_create();
	ram->sg_data = signal_group_create();

	for (int i = 0; i < num_address_lines; ++i) {
		SIGNAL_DEFINE_GROUP(A0 + i, address);
	}

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(D0 + i, data);
	}


	SIGNAL_DEFINE(CE_B);
	SIGNAL_DEFINE(WE_B);
	SIGNAL_DEFINE(OE_B);

	return ram;
}

static void ram_8d16a_register_dependencies(Ram8d16a *ram) {
	assert(ram);
	for (int i = 0; i < arrlen(ram->sg_address); ++i) {
		signal_add_dependency(ram->signal_pool, ram->sg_address[i], ram->id);
	}
	for (int i = 0; i < arrlen(ram->sg_data); ++i) {
		signal_add_dependency(ram->signal_pool, ram->sg_data[i], ram->id);
	}

	SIGNAL_DEPENDENCY(CE_B);
	SIGNAL_DEPENDENCY(WE_B);
	SIGNAL_DEPENDENCY(OE_B);
}

static void ram_8d16a_destroy(Ram8d16a *ram) {
	assert(ram);
	free(ram);
}

static void ram_8d16a_process(Ram8d16a *ram) {
	assert(ram);

	if (!ACTLO_ASSERTED(SIGNAL_READ(CE_B))) {
		SIGNAL_GROUP_NO_WRITE(data);
		return;
	}

	uint16_t address = SIGNAL_GROUP_READ_U16(address);

	if (ACTLO_ASSERTED(SIGNAL_READ(OE_B))) {
		SIGNAL_GROUP_WRITE(data, ram->data_array[address]);
	} else {
		SIGNAL_GROUP_NO_WRITE(data);
		if (ACTLO_ASSERTED(SIGNAL_READ(WE_B))) {
			ram->data_array[address] = SIGNAL_GROUP_READ_U8(data);
		}
	}
}
