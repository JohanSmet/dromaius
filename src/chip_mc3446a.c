// chip_mc3446a.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the Motorala MC3446A (Quad Interface Bus Transceiver)

#include "chip_mc3446a.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_PREFIX		CHIP_MC3446A_
#define SIGNAL_OWNER		chip

static uint8_t ChipMC3446A_PinTypes[CHIP_MC3446A_PIN_COUNT] = {
	[CHIP_MC3446A_AO    ] = CHIP_PIN_OUTPUT,
	[CHIP_MC3446A_AB    ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_MC3446A_AI    ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_MC3446A_ABCE_B] = CHIP_PIN_INPUT,
	[CHIP_MC3446A_BI    ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_MC3446A_BB    ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_MC3446A_BO    ] = CHIP_PIN_OUTPUT,
	[CHIP_MC3446A_CO    ] = CHIP_PIN_OUTPUT,
	[CHIP_MC3446A_CB    ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_MC3446A_CI    ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_MC3446A_DE_B  ] = CHIP_PIN_INPUT,
	[CHIP_MC3446A_DI    ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_MC3446A_DB    ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_MC3446A_DO    ] = CHIP_PIN_OUTPUT,
};

void chip_mc3446a_destroy(ChipMC3446A *chip);
void chip_mc3446a_process(ChipMC3446A *chip);

ChipMC3446A *chip_mc3446a_create(Simulator *sim, ChipMC3446ASignals signals) {
	ChipMC3446A *chip = (ChipMC3446A *) calloc(1, sizeof(ChipMC3446A));

	CHIP_SET_FUNCTIONS(chip, chip_mc3446a_process, chip_mc3446a_destroy);
	CHIP_SET_VARIABLES(chip, sim, chip->signals, ChipMC3446A_PinTypes, CHIP_MC3446A_PIN_COUNT);

	chip->signal_pool = sim->signal_pool;

	memcpy(chip->signals, signals, sizeof(ChipMC3446ASignals));

	SIGNAL_DEFINE(AO);
	SIGNAL_DEFINE(AB);
	SIGNAL_DEFINE(AI);
	SIGNAL_DEFINE(ABCE_B);
	SIGNAL_DEFINE(BI);
	SIGNAL_DEFINE(BB);
	SIGNAL_DEFINE(BO);
	SIGNAL_DEFINE(CO);
	SIGNAL_DEFINE(CB);
	SIGNAL_DEFINE(CI);
	SIGNAL_DEFINE(DE_B);
	SIGNAL_DEFINE(DI);
	SIGNAL_DEFINE(DB);
	SIGNAL_DEFINE(DO);

	signal_default(SIGNAL_POOL, SIGNAL(AB), true);
	signal_default(SIGNAL_POOL, SIGNAL(BB), true);
	signal_default(SIGNAL_POOL, SIGNAL(CB), true);
	signal_default(SIGNAL_POOL, SIGNAL(DB), true);

	return chip;
}

void chip_mc3446a_destroy(ChipMC3446A *chip) {
	assert(chip);
	free(chip);
}

void chip_mc3446a_process(ChipMC3446A *chip) {
	assert(chip);

	bool enable_abc_b = SIGNAL_READ(ABCE_B);

	// transceiver-A
	if (!(enable_abc_b | SIGNAL_READ(AI))) {
		SIGNAL_WRITE(AB, false);
		SIGNAL_WRITE(AO, false);
	} else {
		SIGNAL_NO_WRITE(AB);
		SIGNAL_WRITE(AO, SIGNAL_READ(AB));
	}

	// transceiver-B
	if (!(enable_abc_b | SIGNAL_READ(BI))) {
		SIGNAL_WRITE(BB, false);
		SIGNAL_WRITE(BO, false);
	} else {
		SIGNAL_NO_WRITE(BB);
		SIGNAL_WRITE(BO, SIGNAL_READ(BB));
	}

	// transceiver-C
	if (!(enable_abc_b | SIGNAL_READ(CI))) {
		SIGNAL_WRITE(CB, false);
		SIGNAL_WRITE(CO, false);
	} else {
		SIGNAL_NO_WRITE(CB);
		SIGNAL_WRITE(CO, SIGNAL_READ(CB));
	}

	// transceiver-D
	if (!(SIGNAL_READ(DE_B) || SIGNAL_READ(DI))) {
		SIGNAL_WRITE(DB, false);
		SIGNAL_WRITE(DO, false);
	} else {
		SIGNAL_NO_WRITE(DB);
		SIGNAL_WRITE(DO, SIGNAL_READ(DB));
	}
}


