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

static uint8_t Chip6316Rom_PinTypes[CHIP_63XX_PIN_COUNT] = {
	[CHIP_6316_CS1_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_CS2_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_CS3  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A0   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A1   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A2   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A3   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A4   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A5   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A6   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A7   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A8   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A9   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_A10  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6316_D0   ] = CHIP_PIN_OUTPUT,
	[CHIP_6316_D1   ] = CHIP_PIN_OUTPUT,
	[CHIP_6316_D2   ] = CHIP_PIN_OUTPUT,
	[CHIP_6316_D3   ] = CHIP_PIN_OUTPUT,
	[CHIP_6316_D4   ] = CHIP_PIN_OUTPUT,
	[CHIP_6316_D5   ] = CHIP_PIN_OUTPUT,
	[CHIP_6316_D6   ] = CHIP_PIN_OUTPUT,
	[CHIP_6316_D7   ] = CHIP_PIN_OUTPUT,
};

static void chip_63xx_rom_destroy(Chip63xxRom *chip);
static void chip_6316_rom_process(Chip63xxRom *chip);

Chip63xxRom *chip_6316_rom_create(Simulator *sim, Chip63xxSignals signals) {

	Chip63xxRom *chip = (Chip63xxRom *) calloc(1, sizeof(Chip63xxRom) + ROM_6316_DATA_SIZE);

	CHIP_SET_FUNCTIONS(chip, chip_6316_rom_process, chip_63xx_rom_destroy);
	CHIP_SET_VARIABLES(chip, sim, chip->signals, Chip6316Rom_PinTypes, CHIP_63XX_PIN_COUNT);
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

static uint8_t Chip6332Rom_PinTypes[CHIP_63XX_PIN_COUNT] = {
	[CHIP_6332_CS1_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_CS3  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A0   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A1   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A2   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A3   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A4   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A5   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A6   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A7   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A8   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A9   ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A10  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_A11  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_6332_D0   ] = CHIP_PIN_OUTPUT,
	[CHIP_6332_D1   ] = CHIP_PIN_OUTPUT,
	[CHIP_6332_D2   ] = CHIP_PIN_OUTPUT,
	[CHIP_6332_D3   ] = CHIP_PIN_OUTPUT,
	[CHIP_6332_D4   ] = CHIP_PIN_OUTPUT,
	[CHIP_6332_D5   ] = CHIP_PIN_OUTPUT,
	[CHIP_6332_D6   ] = CHIP_PIN_OUTPUT,
	[CHIP_6332_D7   ] = CHIP_PIN_OUTPUT,
};

static void chip_6332_rom_process(Chip63xxRom *chip);

Chip63xxRom *chip_6332_rom_create(Simulator *sim, Chip63xxSignals signals) {

	Chip63xxRom *chip = (Chip63xxRom *) calloc(1, sizeof(Chip63xxRom) + ROM_6332_DATA_SIZE);

	CHIP_SET_FUNCTIONS(chip, chip_6332_rom_process, chip_63xx_rom_destroy);
	CHIP_SET_VARIABLES(chip, sim, chip->signals, Chip6332Rom_PinTypes, CHIP_63XX_PIN_COUNT);

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

