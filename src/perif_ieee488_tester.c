// perif_ieee488_tester.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// A periphiral to test/monitor the IEEE-488 bus

#include "perif_ieee488_tester.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>

#define SIGNAL_PREFIX		CHIP_488TEST_
#define SIGNAL_OWNER		chip

static uint8_t Perif488Tester_PinTypes[CHIP_488TEST_PIN_COUNT] = {
	[CHIP_488TEST_EOI_B ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_488TEST_DAV_B ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_488TEST_NRFD_B] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_488TEST_NDAC_B] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_488TEST_ATN_B ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_488TEST_SRQ_B ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_488TEST_IFC_B ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,

	[CHIP_488TEST_DIO0  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[CHIP_488TEST_DIO1  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[CHIP_488TEST_DIO2  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[CHIP_488TEST_DIO3  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[CHIP_488TEST_DIO4  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[CHIP_488TEST_DIO5  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[CHIP_488TEST_DIO6  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[CHIP_488TEST_DIO7  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
};

static void perif_ieee488_tester_destroy(Perif488Tester *chip);
static void perif_ieee488_tester_process(Perif488Tester *chip);

Perif488Tester *perif_ieee488_tester_create(Simulator *sim, Perif488TesterSignals signals) {
	Perif488Tester *chip = (Perif488Tester *) calloc(1, sizeof(Perif488Tester));

	// chip
	CHIP_SET_FUNCTIONS(chip, perif_ieee488_tester_process, perif_ieee488_tester_destroy);
	CHIP_SET_VARIABLES(chip, sim, chip->signals, Perif488Tester_PinTypes, CHIP_488TEST_PIN_COUNT);

	// signals
	chip->signal_pool = sim->signal_pool;
	memcpy(chip->signals, signals, sizeof(Perif488TesterSignals));

	SIGNAL_DEFINE_DEFAULT(EOI_B, true);
	SIGNAL_DEFINE_DEFAULT(DAV_B, true);
	SIGNAL_DEFINE_DEFAULT(NRFD_B, true);
	SIGNAL_DEFINE_DEFAULT(NDAC_B, true);
	SIGNAL_DEFINE_DEFAULT(ATN_B, true);
	SIGNAL_DEFINE_DEFAULT(SRQ_B, true);
	SIGNAL_DEFINE_DEFAULT(IFC_B, true);

	SIGNAL_DEFINE(DIO0);
	SIGNAL_DEFINE_DEFAULT(DIO1, true);
	SIGNAL_DEFINE_DEFAULT(DIO2, true);
	SIGNAL_DEFINE_DEFAULT(DIO3, true);
	SIGNAL_DEFINE_DEFAULT(DIO4, true);
	SIGNAL_DEFINE_DEFAULT(DIO5, true);
	SIGNAL_DEFINE_DEFAULT(DIO6, true);
	SIGNAL_DEFINE_DEFAULT(DIO7, true);

	return chip;
}

static void perif_ieee488_tester_destroy(Perif488Tester *chip) {
	assert(chip);
	free(chip);
}

static void perif_ieee488_tester_process(Perif488Tester *chip) {
	assert(chip);

	for (int i = 0; i < CHIP_488TEST_PIN_COUNT; ++i) {
		if (chip->force_output_low[i]) {
			signal_write(SIGNAL_POOL, chip->signals[i], false);
		} else {
			signal_clear_writer(SIGNAL_POOL, chip->signals[i]);
		}
	}

	chip->schedule_timestamp = chip->simulator->current_tick + simulator_interval_to_tick_count(chip->simulator, MS_TO_PS(100));
}
