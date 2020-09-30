// chip_ram_static.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of various static random access memory chips

#include "chip_ram_static.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			chip->signal_pool
#define SIGNAL_COLLECTION	chip->signals
#define SIGNAL_CHIP_ID		chip->id

///////////////////////////////////////////////////////////////////////////////
//
// UM6114 - 1K x 4bits Static Random Access Memory
//

Chip6114SRam *chip_6114_sram_create(Simulator *sim, Chip6114SRamSignals signals) {
	Chip6114SRam *chip = (Chip6114SRam *) calloc(1, sizeof(Chip6114SRam));
	chip->simulator = sim;
	chip->signal_pool = sim->signal_pool;

	CHIP_SET_FUNCTIONS(chip, chip_6114_sram_process, chip_6114_sram_destroy, chip_6114_sram_register_dependencies);

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(bus_address, 10);
	SIGNAL_DEFINE(bus_io, 4);
	SIGNAL_DEFINE(ce_b, 1);
	SIGNAL_DEFINE(rw, 1);

	return chip;
}

void chip_6114_sram_register_dependencies(Chip6114SRam *chip) {
	assert(chip);
	signal_add_dependency(chip->signal_pool, SIGNAL(bus_address), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(bus_io), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(ce_b), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(rw), chip->id);
}

void chip_6114_sram_destroy(Chip6114SRam *chip) {
	assert(chip);
	free(chip);
}

void chip_6114_sram_process(Chip6114SRam *chip) {

	if (!ACTLO_ASSERTED(SIGNAL_BOOL(ce_b))) {
		SIGNAL_NO_WRITE(bus_io);
		return;
	}

	int addr = SIGNAL_UINT16(bus_address);

	if (SIGNAL_BOOL(rw)) {
		SIGNAL_SET_UINT8(bus_io, chip->data_array[addr]);
	} else {
		chip->data_array[addr] = SIGNAL_UINT8(bus_io);
		SIGNAL_NO_WRITE(bus_io);
	}
}


