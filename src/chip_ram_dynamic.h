// chip_ram_dynamic.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of various dynamic random access memory chips

#ifndef DROMAIUS_RAM_DYNAMIC_H
#define DROMAIUS_RAM_DYNAMIC_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types - 8x MK 4116 (16K x 1 Bit Dynamic Ram)
//	- 8 parallel chips in one type because I don't want to waste 8x the memory
//	- does not emulate refresh cycle (or lack thereof)
//  - does not follow real pin-assignments because it aggregates 8 chips

enum Chip8x4116DRamSignalAssignment {
	// 7-bit address
	CHIP_4116_A0 = CHIP_PIN_01,
	CHIP_4116_A1 = CHIP_PIN_02,
	CHIP_4116_A2 = CHIP_PIN_03,
	CHIP_4116_A3 = CHIP_PIN_04,
	CHIP_4116_A4 = CHIP_PIN_05,
	CHIP_4116_A5 = CHIP_PIN_06,
	CHIP_4116_A6 = CHIP_PIN_07,

	// 8-bit data in
	CHIP_4116_DI0 = CHIP_PIN_08,
	CHIP_4116_DI1 = CHIP_PIN_09,
	CHIP_4116_DI2 = CHIP_PIN_10,
	CHIP_4116_DI3 = CHIP_PIN_11,
	CHIP_4116_DI4 = CHIP_PIN_12,
	CHIP_4116_DI5 = CHIP_PIN_13,
	CHIP_4116_DI6 = CHIP_PIN_14,
	CHIP_4116_DI7 = CHIP_PIN_15,

	// 8-bit data out
	CHIP_4116_DO0 = CHIP_PIN_16,
	CHIP_4116_DO1 = CHIP_PIN_17,
	CHIP_4116_DO2 = CHIP_PIN_18,
	CHIP_4116_DO3 = CHIP_PIN_19,
	CHIP_4116_DO4 = CHIP_PIN_20,
	CHIP_4116_DO5 = CHIP_PIN_21,
	CHIP_4116_DO6 = CHIP_PIN_22,
	CHIP_4116_DO7 = CHIP_PIN_23,

	CHIP_4116_WE_B = CHIP_PIN_24,	// 1-bit write enable (active low)
	CHIP_4116_RAS_B = CHIP_PIN_25,  // 1-bit row-address-select (active low)
	CHIP_4116_CAS_B = CHIP_PIN_26,  // 1-bit column-address-select (active low)
};

#define CHIP_4116_PIN_COUNT 26
typedef Signal Chip8x4116DRamSignals[CHIP_4116_PIN_COUNT];

typedef struct Chip8x4116DRam {

	CHIP_DECLARE_BASE

	// interface
	SignalPool *		signal_pool;
	Chip8x4116DRamSignals	signals;

	SignalGroup			sg_address;
	SignalGroup			sg_din;
	SignalGroup			sg_dout;

	// config
	int64_t		access_time;

	// data
	uint8_t		row;
	uint8_t		col;
	uint8_t		do_latch;
	uint8_t		data_array[128 * 128];

	int			state;
	int64_t		next_state_transition;
} Chip8x4116DRam;

// functions - 8x MK 4116
Chip8x4116DRam *chip_8x4116_dram_create(struct Simulator *simulator, Chip8x4116DRamSignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_RAM_DYNAMIC_H
