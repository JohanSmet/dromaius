// chip_dummy.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// 'Dummy' test that can be used in unit tests

#ifndef DROMAIUS_CHIP_DUMMY_H
#define DROMAIUS_CHIP_DUMMY_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
enum ChipDummySignalAssignment {
	CHIP_DUMMY_I0 = CHIP_PIN_01,
	CHIP_DUMMY_I1 = CHIP_PIN_02,
	CHIP_DUMMY_I2 = CHIP_PIN_03,
	CHIP_DUMMY_I3 = CHIP_PIN_04,
	CHIP_DUMMY_I4 = CHIP_PIN_05,
	CHIP_DUMMY_I5 = CHIP_PIN_06,
	CHIP_DUMMY_I6 = CHIP_PIN_07,
	CHIP_DUMMY_I7 = CHIP_PIN_08,

	CHIP_DUMMY_O0 = CHIP_PIN_09,
	CHIP_DUMMY_O1 = CHIP_PIN_10,
	CHIP_DUMMY_O2 = CHIP_PIN_11,
	CHIP_DUMMY_O3 = CHIP_PIN_12,
	CHIP_DUMMY_O4 = CHIP_PIN_13,
	CHIP_DUMMY_O5 = CHIP_PIN_14,
	CHIP_DUMMY_O6 = CHIP_PIN_15,
	CHIP_DUMMY_O7 = CHIP_PIN_16,

	CHIP_DUMMY_PIN_COUNT
};

typedef struct ChipDummy {
	CHIP_DECLARE_BASE

	SignalPool *		signal_pool;
	Signal				signals[CHIP_DUMMY_PIN_COUNT];

} ChipDummy;

// functions
ChipDummy *chip_dummy_create(struct Simulator *sim, Signal signals[CHIP_DUMMY_PIN_COUNT]);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_DUMMY_H
