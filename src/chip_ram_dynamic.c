// chip_ram_dynamic.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of various dynamic random access memory chips

#include "chip_ram_dynamic.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			chip->signal_pool
#define SIGNAL_COLLECTION	chip->signals
#define SIGNAL_CHIP_ID		chip->id

///////////////////////////////////////////////////////////////////////////////
//
// 8x MK 4116 (16K x 1 Bit Dynamic Ram)
//

#define CHIP_8x4116_IDLE 0
#define CHIP_8x4116_OUTPUT_BEGIN 1
#define CHIP_8x4116_OUTPUT 2

void chip_8x4116_dram_register_dependencies(Chip8x4116DRam *chip);
void chip_8x4116_dram_destroy(Chip8x4116DRam *chip);
void chip_8x4116_dram_process(Chip8x4116DRam *chip);

Chip8x4116DRam *chip_8x4116_dram_create(SignalPool *pool, Chip8x4116DRamSignals signals) {
	Chip8x4116DRam *chip = (Chip8x4116DRam *) calloc(1, sizeof(Chip8x4116DRam));
	chip->signal_pool = pool;
	chip->access_time = signal_pool_interval_to_tick_count(chip->signal_pool, NS_TO_PS(100));

	CHIP_SET_FUNCTIONS(chip, chip_8x4116_dram_process, chip_8x4116_dram_destroy, chip_8x4116_dram_register_dependencies);

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(bus_address, 7);
	SIGNAL_DEFINE(we_b, 1);
	SIGNAL_DEFINE(ras_b, 1);
	SIGNAL_DEFINE(cas_b, 1);

	SIGNAL_DEFINE(bus_di, 8);

	static bool fuck = false;
	if (!fuck) {
		SIGNAL_DEFINE_N(bus_do, 8, "IRAMDO%d");
		fuck = true;
	} else {
		SIGNAL_DEFINE_N(bus_do, 8, "JRAMDO%d");
	}

	return chip;
}

void chip_8x4116_dram_register_dependencies(Chip8x4116DRam *chip) {
	assert(chip);
	signal_add_dependency(chip->signal_pool, SIGNAL(ras_b), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(cas_b), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(we_b), chip->id);
}

void chip_8x4116_dram_destroy(Chip8x4116DRam *chip) {
	assert(chip);
	free(chip);
}

void chip_8x4116_dram_process(Chip8x4116DRam *chip) {
	assert(chip);

	bool ras_b = SIGNAL_BOOL(ras_b);
	bool cas_b = SIGNAL_BOOL(cas_b);

	// negative edge on ras_b: latch row address
	if (!ras_b && signal_changed_last_tick(chip->signal_pool, SIGNAL(ras_b))) {
		chip->row = SIGNAL_UINT8(bus_address);
		return;
	}

	// negative adge on cas_b: latch col address
	if (!ras_b && !cas_b && signal_changed_last_tick(chip->signal_pool, SIGNAL(cas_b))) {
		chip->col = SIGNAL_UINT8(bus_address);

		// if write-enable is already asserted: perform early write
		if (ACTLO_ASSERTED(SIGNAL_BOOL(we_b))) {
			chip->data_array[chip->row * 128 + chip->col] = SIGNAL_UINT8(bus_di);
		} else {
			chip->schedule_timestamp = chip->signal_pool->current_tick + chip->access_time;
			chip->next_state_transition = chip->schedule_timestamp;
			chip->state = CHIP_8x4116_OUTPUT_BEGIN;
		}
		return;
	}

	// negative edge on write while ras_b and cas_b are asserted
	if (!ras_b && !cas_b && !SIGNAL_BOOL(we_b) && signal_changed_last_tick(chip->signal_pool, SIGNAL(we_b))) {
		chip->data_array[chip->row * 128 + chip->col] = SIGNAL_UINT8(bus_di);
		return;
	}

	if (chip->state == CHIP_8x4116_OUTPUT_BEGIN && chip->next_state_transition >= chip->signal_pool->current_tick) {
		chip->do_latch = chip->data_array[chip->row * 128 + chip->col];
		SIGNAL_SET_UINT8(bus_do, chip->do_latch);
		chip->state = CHIP_8x4116_OUTPUT;
	}

	if (chip->state == CHIP_8x4116_OUTPUT) {
		SIGNAL_SET_UINT8(bus_do, chip->do_latch);
	}

	if (chip->state == CHIP_8x4116_OUTPUT && cas_b) {
		SIGNAL_NO_WRITE(bus_do);
		chip->state = CHIP_8x4116_IDLE;
	}
}


