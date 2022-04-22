// chip_ram_dynamic.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of various dynamic random access memory chips

#include "chip_ram_dynamic.h"
#include "simulator.h"

#include "crt.h"

#define SIGNAL_PREFIX		CHIP_4116_
#define SIGNAL_OWNER		chip

///////////////////////////////////////////////////////////////////////////////
//
// 8x MK 4116 (16K x 1 Bit Dynamic Ram)
//

static uint8_t Chip8x4116DRam_PinTypes[CHIP_4116_PIN_COUNT] = {
	[CHIP_4116_A0    ] = CHIP_PIN_INPUT,
	[CHIP_4116_A1    ] = CHIP_PIN_INPUT,
	[CHIP_4116_A2    ] = CHIP_PIN_INPUT,
	[CHIP_4116_A3    ] = CHIP_PIN_INPUT,
	[CHIP_4116_A4    ] = CHIP_PIN_INPUT,
	[CHIP_4116_A5    ] = CHIP_PIN_INPUT,
	[CHIP_4116_A6    ] = CHIP_PIN_INPUT,
	[CHIP_4116_DI0   ] = CHIP_PIN_INPUT,
	[CHIP_4116_DI1   ] = CHIP_PIN_INPUT,
	[CHIP_4116_DI2   ] = CHIP_PIN_INPUT,
	[CHIP_4116_DI3   ] = CHIP_PIN_INPUT,
	[CHIP_4116_DI4   ] = CHIP_PIN_INPUT,
	[CHIP_4116_DI5   ] = CHIP_PIN_INPUT,
	[CHIP_4116_DI6   ] = CHIP_PIN_INPUT,
	[CHIP_4116_DI7   ] = CHIP_PIN_INPUT,
	[CHIP_4116_DO0   ] = CHIP_PIN_OUTPUT,
	[CHIP_4116_DO1   ] = CHIP_PIN_OUTPUT,
	[CHIP_4116_DO2   ] = CHIP_PIN_OUTPUT,
	[CHIP_4116_DO3   ] = CHIP_PIN_OUTPUT,
	[CHIP_4116_DO4   ] = CHIP_PIN_OUTPUT,
	[CHIP_4116_DO5   ] = CHIP_PIN_OUTPUT,
	[CHIP_4116_DO6   ] = CHIP_PIN_OUTPUT,
	[CHIP_4116_DO7   ] = CHIP_PIN_OUTPUT,
	[CHIP_4116_WE_B  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_4116_RAS_B ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_4116_CAS_B ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
};

#define CHIP_8x4116_IDLE 0
#define CHIP_8x4116_OUTPUT_BEGIN 1
#define CHIP_8x4116_OUTPUT 2

void chip_8x4116_dram_destroy(Chip8x4116DRam *chip);
void chip_8x4116_dram_process(Chip8x4116DRam *chip);

Chip8x4116DRam *chip_8x4116_dram_create(Simulator *sim, Chip8x4116DRamSignals signals) {
	Chip8x4116DRam *chip = (Chip8x4116DRam *) dms_calloc(1, sizeof(Chip8x4116DRam));

	CHIP_SET_FUNCTIONS(chip, chip_8x4116_dram_process, chip_8x4116_dram_destroy);
	CHIP_SET_VARIABLES(chip, sim, chip->signals, Chip8x4116DRam_PinTypes, CHIP_4116_PIN_COUNT);

	chip->signal_pool = sim->signal_pool;
	chip->access_time = simulator_interval_to_tick_count(sim, NS_TO_PS(100));

	dms_memcpy(chip->signals, signals, sizeof(Chip8x4116DRamSignals));

	chip->sg_address = signal_group_create();
	chip->sg_din = signal_group_create();
	chip->sg_dout = signal_group_create();

	for (int i = 0; i < 7; ++i) {
		SIGNAL_DEFINE_GROUP(A0 + i, address);
	}

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(DI0 + i, din);
	}

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(DO0 + i, dout);
	}

	SIGNAL_DEFINE(WE_B);
	SIGNAL_DEFINE(RAS_B);
	SIGNAL_DEFINE(CAS_B);

	return chip;
}

void chip_8x4116_dram_destroy(Chip8x4116DRam *chip) {
	assert(chip);
	signal_group_destroy(chip->sg_address);
	signal_group_destroy(chip->sg_din);
	signal_group_destroy(chip->sg_dout);
	dms_free(chip);
}

void chip_8x4116_dram_process(Chip8x4116DRam *chip) {
	assert(chip);

	bool ras_b = SIGNAL_READ(RAS_B);
	bool cas_b = SIGNAL_READ(CAS_B);

	// negative edge on ras_b: latch row address
	if (!ras_b && SIGNAL_CHANGED(RAS_B)) {
		chip->row = SIGNAL_GROUP_READ_U8(address);
		return;
	}

	// negative adge on cas_b: latch col address
	if (!ras_b && !cas_b && SIGNAL_CHANGED(CAS_B)) {
		chip->col = SIGNAL_GROUP_READ_U8(address);

		// if write-enable is already asserted: perform early write
		if (ACTLO_ASSERTED(SIGNAL_READ(WE_B))) {
			chip->data_array[chip->row * 128 + chip->col] = SIGNAL_GROUP_READ_U8(din);
		} else {
			chip->schedule_timestamp = chip->simulator->current_tick + chip->access_time;
			chip->next_state_transition = chip->schedule_timestamp;
			chip->state = CHIP_8x4116_OUTPUT_BEGIN;
		}
		return;
	}

	// negative edge on write while ras_b and cas_b are asserted
	if (!ras_b && !cas_b && !SIGNAL_READ(WE_B) && SIGNAL_CHANGED(WE_B)) {
		chip->data_array[chip->row * 128 + chip->col] = SIGNAL_GROUP_READ_U8(din);
		return;
	}

	if (chip->state == CHIP_8x4116_OUTPUT_BEGIN && chip->next_state_transition >= chip->simulator->current_tick) {
		chip->do_latch = chip->data_array[chip->row * 128 + chip->col];
		SIGNAL_GROUP_WRITE(dout, chip->do_latch);
		chip->state = CHIP_8x4116_OUTPUT;
	}

	if (chip->state == CHIP_8x4116_OUTPUT && cas_b) {
		SIGNAL_GROUP_NO_WRITE(dout);
		chip->state = CHIP_8x4116_IDLE;
	}
}


