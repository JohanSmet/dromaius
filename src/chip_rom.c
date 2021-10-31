// chip_rom.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of read-only memory modules

#include "chip_rom.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_OWNER		chip

//////////////////////////////////////////////////////////////////////////////
//
// 6316 - 2k x 8-bit ROM
//

#define SIGNAL_PREFIX		CHIP_6316_

static void chip_63xx_rom_register_dependencies(Chip63xxRom *chip);
static void chip_63xx_rom_destroy(Chip63xxRom *chip);
static void chip_6316_rom_process(Chip63xxRom *chip);

Chip63xxRom *chip_6316_rom_create(Simulator *sim, Chip63xxSignals signals) {

	Chip63xxRom *chip = (Chip63xxRom *) calloc(1, sizeof(Chip63xxRom) + ROM_6316_DATA_SIZE);

	CHIP_SET_FUNCTIONS(chip, chip_6316_rom_process, chip_63xx_rom_destroy, chip_63xx_rom_register_dependencies);
	CHIP_SET_VARIABLES(chip, sim, chip->signals, CHIP_63XX_PIN_COUNT);
	chip->signal_pool = sim->signal_pool;
	chip->data_size = ROM_6316_DATA_SIZE;
	chip->output_delay = simulator_interval_to_tick_count(chip->simulator, NS_TO_PS(60));
	chip->last_address = -1;

	chip->sg_address = signal_group_create();
	chip->sg_data = signal_group_create();

	memcpy(chip->signals, signals, sizeof(Chip63xxSignals));
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
	SIGNAL_DEFINE_GROUP(A10, address);

	SIGNAL_DEFINE_GROUP(D0, data);
	SIGNAL_DEFINE_GROUP(D1, data);
	SIGNAL_DEFINE_GROUP(D2, data);
	SIGNAL_DEFINE_GROUP(D3, data);
	SIGNAL_DEFINE_GROUP(D4, data);
	SIGNAL_DEFINE_GROUP(D5, data);
	SIGNAL_DEFINE_GROUP(D6, data);
	SIGNAL_DEFINE_GROUP(D7, data);

	SIGNAL_DEFINE(CS1_B);
	SIGNAL_DEFINE(CS2_B);
	SIGNAL_DEFINE(CS3);

	return chip;
}

static void chip_63xx_rom_register_dependencies(Chip63xxRom *chip) {
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
	SIGNAL_DEPENDENCY(A10);

	SIGNAL_DEPENDENCY(CS1_B);
	SIGNAL_DEPENDENCY(CS2_B);
	SIGNAL_DEPENDENCY(CS3);
}

static void chip_63xx_rom_destroy(Chip63xxRom *chip) {
	assert(chip);
	free(chip);
}

static void chip_6316_rom_process(Chip63xxRom *chip) {
	assert(chip);

	if (!(ACTLO_ASSERTED(SIGNAL_READ(CS1_B)) && ACTLO_ASSERTED(SIGNAL_READ(CS2_B)) && ACTHI_ASSERTED(SIGNAL_READ(CS3)))) {
		SIGNAL_GROUP_NO_WRITE(data);
		return;
	}

	int address = SIGNAL_GROUP_READ_U32(address);

	if (SIGNAL_CHANGED(CS1_B) || SIGNAL_CHANGED(CS2_B) || SIGNAL_CHANGED(CS3) ||
		address != chip->last_address) {
		chip->last_address = address;
		chip->schedule_timestamp = chip->simulator->current_tick + chip->output_delay;
		return;
	}

	uint8_t data = chip->data_array[address];
	SIGNAL_GROUP_WRITE(data, data);
}

//////////////////////////////////////////////////////////////////////////////
//
// 6332 - 2k x 8-bit ROM
//

#undef SIGNAL_PREFIX
#define SIGNAL_PREFIX		CHIP_6332_

static void chip_6332_rom_process(Chip63xxRom *chip);

Chip63xxRom *chip_6332_rom_create(Simulator *sim, Chip63xxSignals signals) {

	Chip63xxRom *chip = (Chip63xxRom *) calloc(1, sizeof(Chip63xxRom) + ROM_6332_DATA_SIZE);

	CHIP_SET_FUNCTIONS(chip, chip_6332_rom_process, chip_63xx_rom_destroy, chip_63xx_rom_register_dependencies);
	CHIP_SET_VARIABLES(chip, sim, chip->signals, CHIP_63XX_PIN_COUNT);

	chip->signal_pool = sim->signal_pool;
	chip->data_size = ROM_6332_DATA_SIZE;
	chip->output_delay = simulator_interval_to_tick_count(chip->simulator, NS_TO_PS(60));
	chip->last_address = -1;

	chip->sg_address = signal_group_create();
	chip->sg_data = signal_group_create();

	memcpy(chip->signals, signals, sizeof(Chip63xxSignals));
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
	SIGNAL_DEFINE_GROUP(A10, address);
	SIGNAL_DEFINE_GROUP(A11, address);

	SIGNAL_DEFINE_GROUP(D0, data);
	SIGNAL_DEFINE_GROUP(D1, data);
	SIGNAL_DEFINE_GROUP(D2, data);
	SIGNAL_DEFINE_GROUP(D3, data);
	SIGNAL_DEFINE_GROUP(D4, data);
	SIGNAL_DEFINE_GROUP(D5, data);
	SIGNAL_DEFINE_GROUP(D6, data);
	SIGNAL_DEFINE_GROUP(D7, data);

	SIGNAL_DEFINE(CS1_B);
	SIGNAL_DEFINE(CS3);

	return chip;
}

static void chip_6332_rom_process(Chip63xxRom *chip) {
	assert(chip);

	if (!(ACTLO_ASSERTED(SIGNAL_READ(CS1_B)) && ACTHI_ASSERTED(SIGNAL_READ(CS3)))) {
		SIGNAL_GROUP_NO_WRITE(data);
		return;
	}

	int address = SIGNAL_GROUP_READ_U32(address);

	if (SIGNAL_CHANGED(CS1_B) || SIGNAL_CHANGED(CS3) ||
		address != chip->last_address) {
		chip->last_address = address;
		chip->schedule_timestamp = chip->simulator->current_tick + chip->output_delay;
		return;
	}

	uint8_t data = chip->data_array[address];
	SIGNAL_GROUP_WRITE(data, data);
}

