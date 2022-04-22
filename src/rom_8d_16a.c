// rom_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a read-only memory module with an 8-bit wide databus and a maximum of 16 datalines (e.g. a 27c512)

#include "rom_8d_16a.h"
#include "simulator.h"
#include "crt.h"

#define SIGNAL_PREFIX		CHIP_ROM8D16A_
#define SIGNAL_OWNER		rom

static uint8_t Rom8d16a_PinTypes[CHIP_ROM8D16A_PIN_COUNT] = {
	[CHIP_ROM8D16A_CE_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A0  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A1  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A2  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A3  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A4  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A5  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A6  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A7  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A8  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A9  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A10 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A11 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A12 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A13 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A14 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_A15 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_ROM8D16A_D0  ] = CHIP_PIN_OUTPUT,
	[CHIP_ROM8D16A_D1  ] = CHIP_PIN_OUTPUT,
	[CHIP_ROM8D16A_D2  ] = CHIP_PIN_OUTPUT,
	[CHIP_ROM8D16A_D3  ] = CHIP_PIN_OUTPUT,
	[CHIP_ROM8D16A_D4  ] = CHIP_PIN_OUTPUT,
	[CHIP_ROM8D16A_D5  ] = CHIP_PIN_OUTPUT,
	[CHIP_ROM8D16A_D6  ] = CHIP_PIN_OUTPUT,
	[CHIP_ROM8D16A_D7  ] = CHIP_PIN_OUTPUT,
};

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void rom_8d16a_destroy(Rom8d16a *rom);
static void rom_8d16a_process(Rom8d16a *rom);

Rom8d16a *rom_8d16a_create(size_t num_address_lines, Simulator *sim, Rom8d16aSignals signals) {

	size_t data_size = (size_t) 1 << num_address_lines;
	Rom8d16a *rom = (Rom8d16a *) dms_calloc(1, sizeof(Rom8d16a) + data_size);

	CHIP_SET_FUNCTIONS(rom, rom_8d16a_process, rom_8d16a_destroy);
	CHIP_SET_VARIABLES(rom, sim, rom->signals, Rom8d16a_PinTypes, CHIP_ROM8D16A_PIN_COUNT);

	rom->signal_pool = sim->signal_pool;
	rom->data_size = data_size;
	rom->output_delay = simulator_interval_to_tick_count(rom->simulator, NS_TO_PS(60));

	dms_memcpy(rom->signals, signals, sizeof(Rom8d16aSignals));

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

static void rom_8d16a_destroy(Rom8d16a *rom) {
	assert(rom);
	signal_group_destroy(rom->sg_address);
	signal_group_destroy(rom->sg_data);
	dms_free(rom);
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
