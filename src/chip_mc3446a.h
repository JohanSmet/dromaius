// chip_mc3446a.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the Motorala MC3446A (Quad Interface Bus Transceiver)

#ifndef DROMAIUS_MC3446A_H
#define DROMAIUS_MC3446A_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ChipMC3446ASignalAssignment {

	CHIP_MC3446A_AO = CHIP_PIN_01,
	CHIP_MC3446A_AB = CHIP_PIN_02,
	CHIP_MC3446A_AI = CHIP_PIN_03,
	CHIP_MC3446A_ABCE_B = CHIP_PIN_04,		// enable ABC-inputs (active low)
	CHIP_MC3446A_BI = CHIP_PIN_05,
	CHIP_MC3446A_BB = CHIP_PIN_06,
	CHIP_MC3446A_BO = CHIP_PIN_07,
	CHIP_MC3446A_CO = CHIP_PIN_09,
	CHIP_MC3446A_CB = CHIP_PIN_10,
	CHIP_MC3446A_CI = CHIP_PIN_11,
	CHIP_MC3446A_DE_B = CHIP_PIN_12,		// enable D-input (active low)
	CHIP_MC3446A_DI = CHIP_PIN_13,
	CHIP_MC3446A_DB = CHIP_PIN_14,
	CHIP_MC3446A_DO = CHIP_PIN_15,

	CHIP_MC3446A_PIN_COUNT
};

typedef Signal ChipMC3446ASignals[CHIP_MC3446A_PIN_COUNT];

typedef struct ChipMC3446A {

	CHIP_DECLARE_BASE

	// interface
	SignalPool *		signal_pool;
	ChipMC3446ASignals	signals;

} ChipMC3446A;

// functions - 8x MK 4116
ChipMC3446A *chip_mc3446a_create(struct Simulator *simulator, ChipMC3446ASignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_MC3446A_H
