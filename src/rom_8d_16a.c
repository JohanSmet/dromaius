// rom_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a read-only memory module with an 8-bit wide databus and a maximum of 16 datalines (e.g. a 27c512)

#include "rom_8d_16a.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_PREFIX		CHIP_ROM8D16A_
#define SIGNAL_OWNER		rom

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void rom_8d16a_register_dependencies(Rom8d16a *rom);
static void rom_8d16a_destroy(Rom8d16a *rom);
static void rom_8d16a_process(Rom8d16a *rom);

Rom8d16a *rom_8d16a_create(size_t num_address_lines, Simulator *sim, Rom8d16aSignals signals) {

	size_t data_size = (size_t) 1 << num_address_lines;
	Rom8d16a *rom = (Rom8d16a *) calloc(1, sizeof(Rom8d16a) + data_size);

	rom->simulator = sim;
	rom->signal_pool = sim->signal_pool;
	rom->data_size = data_size;
	rom->output_delay = simulator_interval_to_tick_count(rom->simulator, NS_TO_PS(60));
	CHIP_SET_FUNCTIONS(rom, rom_8d16a_process, rom_8d16a_destroy, rom_8d16a_register_dependencies);

	memcpy(rom->signals, signals, sizeof(Rom8d16aSignals));

	rom->sg_address = signal_group_create();
	rom->sg_data = signal_group_create();

	for (size_t i = 0; i < num_address_lines; ++i) {
		SIGNAL_DEFINE_GROUP(A0 + i, address);
	}

	for (size_t i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(D0 + i, data);
	}

	SIGNAL_DEFINE(CE_B);

	return rom;
}

static void rom_8d16a_register_dependencies(Rom8d16a *rom) {
	assert(rom);

	for (int i = 0; i < arrlen(rom->sg_address); ++i) {
		signal_add_dependency(rom->signal_pool, rom->sg_address[i], rom->id);
	}

	SIGNAL_DEPENDENCY(CE_B);
}

static void rom_8d16a_destroy(Rom8d16a *rom) {
	assert(rom);
	free(rom);
}

static void rom_8d16a_process(Rom8d16a *rom) {
	assert(rom);

	if (!ACTLO_ASSERTED(SIGNAL_READ(CE_B))) {
		SIGNAL_GROUP_NO_WRITE(data);
		return;
	}

	uint16_t address = SIGNAL_GROUP_READ_U16(address);

	if (SIGNAL_CHANGED(CE_B) || address != rom->last_address) {
		rom->last_address = address;
		rom->schedule_timestamp = rom->simulator->current_tick + rom->output_delay;
		return;
	}

	SIGNAL_GROUP_WRITE(data, rom->data_array[rom->last_address]);
}
