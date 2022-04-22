// chip_ram_static.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of various static random access memory chips

#include "chip_ram_static.h"
#include "simulator.h"

#include "crt.h"

#define SIGNAL_PREFIX		CHIP_6114_
#define SIGNAL_OWNER		chip

///////////////////////////////////////////////////////////////////////////////
//
// UM6114 - 1K x 4bits Static Random Access Memory
//

static uint8_t Chip6114SRam_PinTypes[CHIP_6114_PIN_COUNT] = {
	[CHIP_6114_A0  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_A1  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_A2  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_A3  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_A4  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_A5  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_A6  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_A7  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_A8  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_A9  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_IO0 ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_IO1 ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_IO2 ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_IO3 ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_CE_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6114_RW  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
};

static void chip_6114_sram_destroy(Chip6114SRam *chip);
static void chip_6114_sram_process(Chip6114SRam *chip);

Chip6114SRam *chip_6114_sram_create(Simulator *sim, Chip6114SRamSignals signals) {
	Chip6114SRam *chip = (Chip6114SRam *) dms_calloc(1, sizeof(Chip6114SRam));

	CHIP_SET_FUNCTIONS(chip, chip_6114_sram_process, chip_6114_sram_destroy);
	CHIP_SET_VARIABLES(chip, sim, chip->signals, Chip6114SRam_PinTypes, CHIP_6114_PIN_COUNT);

	chip->signal_pool = sim->signal_pool;
	dms_memcpy(chip->signals, signals, sizeof(Chip6114SRamSignals));

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

static void chip_6114_sram_destroy(Chip6114SRam *chip) {
	assert(chip);
	signal_group_destroy(chip->sg_address);
	signal_group_destroy(chip->sg_io);
	dms_free(chip);
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


