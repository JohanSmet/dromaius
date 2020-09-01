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

// temporary
#undef  SIGNAL_DEFINE
#define SIGNAL_DEFINE(sig,cnt)										\
	if (chip->signals[(sig)].count == 0) {							\
		chip->signals[(sig)] = signal_create(SIGNAL_POOL, (cnt));		\
	}

#undef  SIGNAL_BOOL
#define SIGNAL_BOOL(sig)	signal_read_bool(SIGNAL_POOL, SIGNAL_COLLECTION[(sig)])

#undef  SIGNAL_SET_BOOL
#define SIGNAL_SET_BOOL(sig,v)	signal_write_bool(SIGNAL_POOL, SIGNAL_COLLECTION[(sig)], (v), SIGNAL_CHIP_ID)

#undef  SIGNAL_NO_WRITE
#define	SIGNAL_NO_WRITE(sig)	signal_clear_writer(SIGNAL_POOL, SIGNAL_COLLECTION[(sig)], SIGNAL_CHIP_ID)

//////////////////////////////////////////////////////////////////////////////
//
// 6316 - 2k x 8-bit ROM
//

void chip_63xx_rom_register_dependencies(Chip63xxRom *chip);
void chip_63xx_rom_destroy(Chip63xxRom *chip);
void chip_6316_rom_process(Chip63xxRom *chip);

Chip63xxRom *chip_6316_rom_create(SignalPool *signal_pool, Chip63xxSignals signals) {

	Chip63xxRom *chip = (Chip63xxRom *) calloc(1, sizeof(Chip63xxRom) + ROM_6316_DATA_SIZE);

	chip->signal_pool = signal_pool;
	chip->data_size = ROM_6316_DATA_SIZE;
	chip->output_delay = signal_pool_interval_to_tick_count(signal_pool, NS_TO_PS(60));
	CHIP_SET_FUNCTIONS(chip, chip_6316_rom_process, chip_63xx_rom_destroy, chip_63xx_rom_register_dependencies);

	memcpy(chip->signals, signals, sizeof(Chip63xxSignals));

	SIGNAL_DEFINE(CHIP_6316_A0, 1);
	SIGNAL_DEFINE(CHIP_6316_A1, 1);
	SIGNAL_DEFINE(CHIP_6316_A2, 1);
	SIGNAL_DEFINE(CHIP_6316_A3, 1);
	SIGNAL_DEFINE(CHIP_6316_A4, 1);
	SIGNAL_DEFINE(CHIP_6316_A5, 1);
	SIGNAL_DEFINE(CHIP_6316_A6, 1);
	SIGNAL_DEFINE(CHIP_6316_A7, 1);
	SIGNAL_DEFINE(CHIP_6316_A8, 1);
	SIGNAL_DEFINE(CHIP_6316_A9, 1);
	SIGNAL_DEFINE(CHIP_6316_A10, 1);

	SIGNAL_DEFINE(CHIP_6316_D0, 1);
	SIGNAL_DEFINE(CHIP_6316_D1, 1);
	SIGNAL_DEFINE(CHIP_6316_D2, 1);
	SIGNAL_DEFINE(CHIP_6316_D3, 1);
	SIGNAL_DEFINE(CHIP_6316_D4, 1);
	SIGNAL_DEFINE(CHIP_6316_D5, 1);
	SIGNAL_DEFINE(CHIP_6316_D6, 1);
	SIGNAL_DEFINE(CHIP_6316_D7, 1);

	SIGNAL_DEFINE(CHIP_6316_CS1_B, 1);
	SIGNAL_DEFINE(CHIP_6316_CS2_B, 1);
	SIGNAL_DEFINE(CHIP_6316_CS3, 1);

	return chip;
}

void chip_63xx_rom_register_dependencies(Chip63xxRom *chip) {
	assert(chip);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A0], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A1], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A2], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A3], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A4], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A5], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A6], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A7], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A8], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A9], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_A10], chip->id);

	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_CS1_B], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_CS2_B], chip->id);
	signal_add_dependency(chip->signal_pool, chip->signals[CHIP_6316_CS3], chip->id);
}

void chip_63xx_rom_destroy(Chip63xxRom *chip) {
	assert(chip);
	free(chip);
}

void chip_6316_rom_process(Chip63xxRom *chip) {
	assert(chip);

	if (!(ACTLO_ASSERTED(SIGNAL_BOOL(CHIP_6316_CS1_B)) && ACTLO_ASSERTED(SIGNAL_BOOL(CHIP_6316_CS2_B)) && ACTHI_ASSERTED(SIGNAL_BOOL(CHIP_6316_CS3)))) {
		SIGNAL_NO_WRITE(CHIP_6316_D0);
		SIGNAL_NO_WRITE(CHIP_6316_D1);
		SIGNAL_NO_WRITE(CHIP_6316_D2);
		SIGNAL_NO_WRITE(CHIP_6316_D3);
		SIGNAL_NO_WRITE(CHIP_6316_D4);
		SIGNAL_NO_WRITE(CHIP_6316_D5);
		SIGNAL_NO_WRITE(CHIP_6316_D6);
		SIGNAL_NO_WRITE(CHIP_6316_D7);
		return;
	}

	int address =	(SIGNAL_BOOL(CHIP_6316_A0) << 0) | (SIGNAL_BOOL(CHIP_6316_A1) << 1) |
					(SIGNAL_BOOL(CHIP_6316_A2) << 2) | (SIGNAL_BOOL(CHIP_6316_A3) << 3) |
					(SIGNAL_BOOL(CHIP_6316_A4) << 4) | (SIGNAL_BOOL(CHIP_6316_A5) << 5) |
					(SIGNAL_BOOL(CHIP_6316_A6) << 6) | (SIGNAL_BOOL(CHIP_6316_A7) << 7) |
					(SIGNAL_BOOL(CHIP_6316_A8) << 8) | (SIGNAL_BOOL(CHIP_6316_A9) << 9) |
					(SIGNAL_BOOL(CHIP_6316_A10) << 10);

	if (signal_changed_last_tick(chip->signal_pool, chip->signals[CHIP_6316_CS1_B]) ||
		signal_changed_last_tick(chip->signal_pool, chip->signals[CHIP_6316_CS2_B]) ||
		signal_changed_last_tick(chip->signal_pool, chip->signals[CHIP_6316_CS3]) ||
		address != chip->last_address) {
		chip->last_address = address;
		chip->schedule_timestamp = chip->signal_pool->current_tick + chip->output_delay;
		return;
	}

	uint8_t data = chip->data_array[address];

	SIGNAL_SET_BOOL(CHIP_6316_D0, (data >> 0) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6316_D1, (data >> 1) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6316_D2, (data >> 2) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6316_D3, (data >> 3) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6316_D4, (data >> 4) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6316_D5, (data >> 5) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6316_D6, (data >> 6) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6316_D7, (data >> 7) & 0x01);
}

//////////////////////////////////////////////////////////////////////////////
//
// 6332 - 2k x 8-bit ROM
//

void chip_6332_rom_process(Chip63xxRom *chip);

Chip63xxRom *chip_6332_rom_create(SignalPool *signal_pool, Chip63xxSignals signals) {

	Chip63xxRom *chip = (Chip63xxRom *) calloc(1, sizeof(Chip63xxRom) + ROM_6332_DATA_SIZE);

	chip->signal_pool = signal_pool;
	chip->data_size = ROM_6332_DATA_SIZE;
	chip->output_delay = signal_pool_interval_to_tick_count(signal_pool, NS_TO_PS(60));
	CHIP_SET_FUNCTIONS(chip, chip_6332_rom_process, chip_63xx_rom_destroy, chip_63xx_rom_register_dependencies);

	memcpy(chip->signals, signals, sizeof(Chip63xxSignals));

	SIGNAL_DEFINE(CHIP_6332_A0, 1);
	SIGNAL_DEFINE(CHIP_6332_A1, 1);
	SIGNAL_DEFINE(CHIP_6332_A2, 1);
	SIGNAL_DEFINE(CHIP_6332_A3, 1);
	SIGNAL_DEFINE(CHIP_6332_A4, 1);
	SIGNAL_DEFINE(CHIP_6332_A5, 1);
	SIGNAL_DEFINE(CHIP_6332_A6, 1);
	SIGNAL_DEFINE(CHIP_6332_A7, 1);
	SIGNAL_DEFINE(CHIP_6332_A8, 1);
	SIGNAL_DEFINE(CHIP_6332_A9, 1);
	SIGNAL_DEFINE(CHIP_6332_A10, 1);
	SIGNAL_DEFINE(CHIP_6332_A11, 1);

	SIGNAL_DEFINE(CHIP_6332_D0, 1);
	SIGNAL_DEFINE(CHIP_6332_D1, 1);
	SIGNAL_DEFINE(CHIP_6332_D2, 1);
	SIGNAL_DEFINE(CHIP_6332_D3, 1);
	SIGNAL_DEFINE(CHIP_6332_D4, 1);
	SIGNAL_DEFINE(CHIP_6332_D5, 1);
	SIGNAL_DEFINE(CHIP_6332_D6, 1);
	SIGNAL_DEFINE(CHIP_6332_D7, 1);

	SIGNAL_DEFINE(CHIP_6332_CS1_B, 1);
	SIGNAL_DEFINE(CHIP_6332_CS3, 1);

	return chip;
}

#if 0
void chip_6332_rom_register_dependencies(Chip6332Rom *chip) {
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
	signal_add_dependency(chip->signal_pool, SIGNAL(a11), chip->id);

	signal_add_dependency(chip->signal_pool, SIGNAL(cs1_b), chip->id);
	signal_add_dependency(chip->signal_pool, SIGNAL(cs3), chip->id);
}

void chip_6332_rom_destroy(Chip6332Rom *chip) {
	assert(chip);
	free(chip);
}

#endif

void chip_6332_rom_process(Chip63xxRom *chip) {
	assert(chip);

	if (!(ACTLO_ASSERTED(SIGNAL_BOOL(CHIP_6332_CS1_B)) && ACTHI_ASSERTED(SIGNAL_BOOL(CHIP_6332_CS3)))) {
		SIGNAL_NO_WRITE(CHIP_6332_D0);
		SIGNAL_NO_WRITE(CHIP_6332_D1);
		SIGNAL_NO_WRITE(CHIP_6332_D2);
		SIGNAL_NO_WRITE(CHIP_6332_D3);
		SIGNAL_NO_WRITE(CHIP_6332_D4);
		SIGNAL_NO_WRITE(CHIP_6332_D5);
		SIGNAL_NO_WRITE(CHIP_6332_D6);
		SIGNAL_NO_WRITE(CHIP_6332_D7);
		return;
	}

	int address =	(SIGNAL_BOOL(CHIP_6332_A0) << 0) | (SIGNAL_BOOL(CHIP_6332_A1) << 1) |
					(SIGNAL_BOOL(CHIP_6332_A2) << 2) | (SIGNAL_BOOL(CHIP_6332_A3) << 3) |
					(SIGNAL_BOOL(CHIP_6332_A4) << 4) | (SIGNAL_BOOL(CHIP_6332_A5) << 5) |
					(SIGNAL_BOOL(CHIP_6332_A6) << 6) | (SIGNAL_BOOL(CHIP_6332_A7) << 7) |
					(SIGNAL_BOOL(CHIP_6332_A8) << 8) | (SIGNAL_BOOL(CHIP_6332_A9) << 9) |
					(SIGNAL_BOOL(CHIP_6332_A10) << 10) | (SIGNAL_BOOL(CHIP_6332_A11) << 11);

	if (signal_changed_last_tick(chip->signal_pool, chip->signals[CHIP_6332_CS1_B]) ||
		signal_changed_last_tick(chip->signal_pool, chip->signals[CHIP_6332_CS3]) ||
		address != chip->last_address) {
		chip->last_address = address;
		chip->schedule_timestamp = chip->signal_pool->current_tick + chip->output_delay;
		return;
	}

	uint8_t data = chip->data_array[address];

	SIGNAL_SET_BOOL(CHIP_6332_D0, (data >> 0) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6332_D1, (data >> 1) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6332_D2, (data >> 2) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6332_D3, (data >> 3) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6332_D4, (data >> 4) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6332_D5, (data >> 5) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6332_D6, (data >> 6) & 0x01);
	SIGNAL_SET_BOOL(CHIP_6332_D7, (data >> 7) & 0x01);
}

