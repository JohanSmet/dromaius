// chip_rom.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of read-only memory modules

#include "chip_rom.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			chip->signal_pool
#define SIGNAL_COLLECTION	chip->signals
#define SIGNAL_CHIP_ID		chip->id

//////////////////////////////////////////////////////////////////////////////
//
// 6316 - 2k x 8-bit ROM
//

Chip6316Rom *chip_6316_rom_create(SignalPool *signal_pool, Chip6316Signals signals) {

	Chip6316Rom *chip = (Chip6316Rom *) calloc(1, sizeof(Chip6316Rom));

	chip->signal_pool = signal_pool;
	CHIP_SET_FUNCTIONS(chip, chip_6316_rom_process, chip_6316_rom_destroy, chip_6316_rom_register_dependencies);

	memcpy(&chip->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(a0, 1);
	SIGNAL_DEFINE(a1, 1);
	SIGNAL_DEFINE(a2, 1);
	SIGNAL_DEFINE(a3, 1);
	SIGNAL_DEFINE(a4, 1);
	SIGNAL_DEFINE(a5, 1);
	SIGNAL_DEFINE(a6, 1);
	SIGNAL_DEFINE(a7, 1);
	SIGNAL_DEFINE(a8, 1);
	SIGNAL_DEFINE(a9, 1);
	SIGNAL_DEFINE(a10, 1);

	SIGNAL_DEFINE(d0, 1);
	SIGNAL_DEFINE(d1, 1);
	SIGNAL_DEFINE(d2, 1);
	SIGNAL_DEFINE(d3, 1);
	SIGNAL_DEFINE(d4, 1);
	SIGNAL_DEFINE(d5, 1);
	SIGNAL_DEFINE(d6, 1);
	SIGNAL_DEFINE(d7, 1);

	SIGNAL_DEFINE(cs1_b, 1);
	SIGNAL_DEFINE(cs2_b, 1);
	SIGNAL_DEFINE(cs3, 1);

	return chip;
}

void chip_6316_rom_register_dependencies(Chip6316Rom *chip) {
	assert(chip);
	signal_add_dependency(chip->signal_pool, SIGNAL(a0), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(a1), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(a2), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(a3), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(a4), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(a5), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(a6), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(a7), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(a8), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(a9), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(a10), chip->id);

	signal_add_dependency(chip->signal_pool, SIGNAL(cs1_b), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(cs2_b), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(cs3), chip->id);
}

void chip_6316_rom_destroy(Chip6316Rom *chip) {
	assert(chip);
	free(chip);
}

void chip_6316_rom_process(Chip6316Rom *chip) {
	assert(chip);

	if (!(ACTLO_ASSERTED(SIGNAL_BOOL(cs1_b)) && ACTLO_ASSERTED(SIGNAL_BOOL(cs2_b)) && ACTHI_ASSERTED(SIGNAL_BOOL(cs3)))) {
		SIGNAL_NO_WRITE(d0);
		SIGNAL_NO_WRITE(d1);
		SIGNAL_NO_WRITE(d2);
		SIGNAL_NO_WRITE(d3);
		SIGNAL_NO_WRITE(d4);
		SIGNAL_NO_WRITE(d5);
		SIGNAL_NO_WRITE(d6);
		SIGNAL_NO_WRITE(d7);
		return;
	}

	int address =	(SIGNAL_BOOL(a0) << 0) | (SIGNAL_BOOL(a1) << 1) |
					(SIGNAL_BOOL(a2) << 2) | (SIGNAL_BOOL(a3) << 3) |
					(SIGNAL_BOOL(a4) << 4) | (SIGNAL_BOOL(a5) << 5) |
					(SIGNAL_BOOL(a6) << 6) | (SIGNAL_BOOL(a7) << 7) |
					(SIGNAL_BOOL(a8) << 8) | (SIGNAL_BOOL(a9) << 9) |
					(SIGNAL_BOOL(a10) << 10);

	uint8_t data = chip->data_array[address];

	SIGNAL_SET_BOOL(d0, (data >> 0) & 0x01);
	SIGNAL_SET_BOOL(d1, (data >> 1) & 0x01);
	SIGNAL_SET_BOOL(d2, (data >> 2) & 0x01);
	SIGNAL_SET_BOOL(d3, (data >> 3) & 0x01);
	SIGNAL_SET_BOOL(d4, (data >> 4) & 0x01);
	SIGNAL_SET_BOOL(d5, (data >> 5) & 0x01);
	SIGNAL_SET_BOOL(d6, (data >> 6) & 0x01);
	SIGNAL_SET_BOOL(d7, (data >> 7) & 0x01);
}
