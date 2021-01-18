// chip_ram_static.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of various static random access memory chips

#define SIGNAL_ARRAY_STYLE
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
	chip->simulator = sim;
	chip->signal_pool = sim->signal_pool;

	CHIP_SET_FUNCTIONS(chip, chip_6114_sram_process, chip_6114_sram_destroy, chip_6114_sram_register_dependencies);

	memcpy(chip->signals, signals, sizeof(Chip6114SRamSignals));

	SIGNAL_DEFINE(CHIP_6114_A0);
	SIGNAL_DEFINE(CHIP_6114_A1);
	SIGNAL_DEFINE(CHIP_6114_A2);
	SIGNAL_DEFINE(CHIP_6114_A3);
	SIGNAL_DEFINE(CHIP_6114_A4);
	SIGNAL_DEFINE(CHIP_6114_A5);
	SIGNAL_DEFINE(CHIP_6114_A6);
	SIGNAL_DEFINE(CHIP_6114_A7);
	SIGNAL_DEFINE(CHIP_6114_A8);
	SIGNAL_DEFINE(CHIP_6114_A9);

	SIGNAL_DEFINE(CHIP_6114_IO0);
	SIGNAL_DEFINE(CHIP_6114_IO1);
	SIGNAL_DEFINE(CHIP_6114_IO2);
	SIGNAL_DEFINE(CHIP_6114_IO3);

	SIGNAL_DEFINE(CHIP_6114_CE_B);
	SIGNAL_DEFINE(CHIP_6114_RW);

	chip->sg_address = signal_group_create();
	signal_group_push(&chip->sg_address, SIGNAL(A0));
	signal_group_push(&chip->sg_address, SIGNAL(A1));
	signal_group_push(&chip->sg_address, SIGNAL(A2));
	signal_group_push(&chip->sg_address, SIGNAL(A3));
	signal_group_push(&chip->sg_address, SIGNAL(A4));
	signal_group_push(&chip->sg_address, SIGNAL(A5));
	signal_group_push(&chip->sg_address, SIGNAL(A6));
	signal_group_push(&chip->sg_address, SIGNAL(A7));
	signal_group_push(&chip->sg_address, SIGNAL(A8));
	signal_group_push(&chip->sg_address, SIGNAL(A9));

	chip->sg_io = signal_group_create();
	signal_group_push(&chip->sg_io, SIGNAL(IO0));
	signal_group_push(&chip->sg_io, SIGNAL(IO1));
	signal_group_push(&chip->sg_io, SIGNAL(IO2));
	signal_group_push(&chip->sg_io, SIGNAL(IO3));

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


