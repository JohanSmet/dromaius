// chip_dummy.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// 'Dummy' test that can be used in unit tests

#include "chip_dummy.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_PREFIX		CHIP_DUMMY_
#define SIGNAL_OWNER		chip

static uint8_t ChipDummy_PinTypes[CHIP_DUMMY_PIN_COUNT] = {
	[CHIP_DUMMY_I0] = CHIP_PIN_INPUT,
	[CHIP_DUMMY_I1] = CHIP_PIN_INPUT,
	[CHIP_DUMMY_I2] = CHIP_PIN_INPUT,
	[CHIP_DUMMY_I3] = CHIP_PIN_INPUT,
	[CHIP_DUMMY_I4] = CHIP_PIN_INPUT,
	[CHIP_DUMMY_I5] = CHIP_PIN_INPUT,
	[CHIP_DUMMY_I6] = CHIP_PIN_INPUT,
	[CHIP_DUMMY_I7] = CHIP_PIN_INPUT,

	[CHIP_DUMMY_O0] = CHIP_PIN_OUTPUT,
	[CHIP_DUMMY_O1] = CHIP_PIN_OUTPUT,
	[CHIP_DUMMY_O2] = CHIP_PIN_OUTPUT,
	[CHIP_DUMMY_O3] = CHIP_PIN_OUTPUT,
	[CHIP_DUMMY_O4] = CHIP_PIN_OUTPUT,
	[CHIP_DUMMY_O5] = CHIP_PIN_OUTPUT,
	[CHIP_DUMMY_O6] = CHIP_PIN_OUTPUT,
	[CHIP_DUMMY_O7] = CHIP_PIN_OUTPUT,
};

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void chip_dummy_destroy(ChipDummy *chip);
static void chip_dummy_process(ChipDummy *chip);

ChipDummy *chip_dummy_create(Simulator *sim, Signal signals[CHIP_DUMMY_PIN_COUNT]) {
	ChipDummy *chip = (ChipDummy *) calloc(1, sizeof(ChipDummy));

	CHIP_SET_FUNCTIONS(chip, chip_dummy_process, chip_dummy_destroy);
	CHIP_SET_VARIABLES(chip, sim, chip->signals, ChipDummy_PinTypes, CHIP_DUMMY_PIN_COUNT);

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(Signal) * CHIP_DUMMY_PIN_COUNT);
	SIGNAL_DEFINE(I0);
	SIGNAL_DEFINE(I1);
	SIGNAL_DEFINE(I2);
	SIGNAL_DEFINE(I3);
	SIGNAL_DEFINE(I4);
	SIGNAL_DEFINE(I5);
	SIGNAL_DEFINE(I6);
	SIGNAL_DEFINE(I7);

	SIGNAL_DEFINE(O0);
	SIGNAL_DEFINE(O1);
	SIGNAL_DEFINE(O2);
	SIGNAL_DEFINE(O3);
	SIGNAL_DEFINE(O4);
	SIGNAL_DEFINE(O5);
	SIGNAL_DEFINE(O6);
	SIGNAL_DEFINE(O7);

	return chip;
}

static void chip_dummy_destroy(ChipDummy *chip) {
	assert(chip);
	free(chip);
}

static void chip_dummy_process(ChipDummy *chip) {
	assert(chip);
	(void) chip;
}

