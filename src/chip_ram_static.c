// chip_ram_static.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of various static random access memory chips

#include "chip_ram_static.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_PREFIX		CHIP_6114_
#define SIGNAL_OWNER		chip

///////////////////////////////////////////////////////////////////////////////
//
// UM6114 - 1K x 4bits Static Random Access Memory
//

static void chip_6114_sram_register_dependencies(Chip6114SRam *chip);
static void chip_6114_sram_destroy(Chip6114SRam *chip);
static void chip_6114_sram_process(Chip6114SRam *chip);

Chip6114SRam *chip_6114_sram_create(Simulator *sim, Chip6114SRamSignals signals) {
	Chip6114SRam *chip = (Chip6114SRam *) calloc(1, sizeof(Chip6114SRam));

	CHIP_SET_FUNCTIONS(chip, chip_6114_sram_process, chip_6114_sram_destroy, chip_6114_sram_register_dependencies);
	CHIP_SET_VARIABLES(chip, sim, chip->signals, CHIP_6114_PIN_COUNT);

	chip->signal_pool = sim->signal_pool;
	memcpy(chip->signals, signals, sizeof(Chip6114SRamSignals));

	SIGNAL_DEFINE_GROUP(A0, address);
	SIGNAL_DEFINE_GROUP(A1, address);
	SIGNAL_DEFINE_GROUP(A2, address);
	SIGNAL_DEFINE_GROUP(A3, address);
	SIGNAL_DEFINE_GROUP(A4, address);
	SIGNAL_DEFINE_GROUP(A5, address);
	SIGNAL_DEFINE_GROUP(A6, address);
	SIGNAL_DEFINE_GROUP(A7, address);
	SIGNAL_DEFINE_GROUP(A8, address);
	SIGNAL_DEFINE_GROUP(A9, address);

	SIGNAL_DEFINE_GROUP(IO0, io);
	SIGNAL_DEFINE_GROUP(IO1, io);
	SIGNAL_DEFINE_GROUP(IO2, io);
	SIGNAL_DEFINE_GROUP(IO3, io);

	SIGNAL_DEFINE(CE_B);
	SIGNAL_DEFINE(RW);

	return chip;
}

static void chip_6114_sram_register_dependencies(Chip6114SRam *chip) {
	assert(chip);
	SIGNAL_DEPENDENCY(A0);
	SIGNAL_DEPENDENCY(A1);
	SIGNAL_DEPENDENCY(A2);
	SIGNAL_DEPENDENCY(A3);
	SIGNAL_DEPENDENCY(A4);
	SIGNAL_DEPENDENCY(A5);
	SIGNAL_DEPENDENCY(A6);
	SIGNAL_DEPENDENCY(A7);
	SIGNAL_DEPENDENCY(A8);
	SIGNAL_DEPENDENCY(A9);
	SIGNAL_DEPENDENCY(IO0);
	SIGNAL_DEPENDENCY(IO1);
	SIGNAL_DEPENDENCY(IO2);
	SIGNAL_DEPENDENCY(IO3);
	SIGNAL_DEPENDENCY(CE_B);
	SIGNAL_DEPENDENCY(RW);
}

static void chip_6114_sram_destroy(Chip6114SRam *chip) {
	assert(chip);
	free(chip);
}

static void chip_6114_sram_process(Chip6114SRam *chip) {

	if (!ACTLO_ASSERTED(SIGNAL_READ(CE_B))) {
		SIGNAL_GROUP_NO_WRITE(io);
		return;
	}

	int addr = SIGNAL_GROUP_READ_U32(address);

	if (SIGNAL_READ(RW)) {
		SIGNAL_GROUP_WRITE(io, chip->data_array[addr]);
	} else {
		chip->data_array[addr] = SIGNAL_GROUP_READ_U8(io);
		SIGNAL_GROUP_NO_WRITE(io);
	}
}


