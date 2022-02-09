// perif_ieee488_tester.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// A periphiral to test/monitor the IEEE-488 bus

#ifndef DROMAIUS_PERIF_IEEE488_TESTER_H
#define DROMAIUS_PERIF_IEEE488_TESTER_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
enum Perif488TesterSignalAssignment {
	CHIP_488TEST_EOI_B = CHIP_PIN_01,
	CHIP_488TEST_DAV_B,
	CHIP_488TEST_NRFD_B,
	CHIP_488TEST_NDAC_B,
	CHIP_488TEST_ATN_B,
	CHIP_488TEST_SRQ_B,
	CHIP_488TEST_IFC_B,

	CHIP_488TEST_DIO0,
	CHIP_488TEST_DIO1,
	CHIP_488TEST_DIO2,
	CHIP_488TEST_DIO3,
	CHIP_488TEST_DIO4,
	CHIP_488TEST_DIO5,
	CHIP_488TEST_DIO6,
	CHIP_488TEST_DIO7,

	CHIP_488TEST_PIN_COUNT
};

typedef Signal Perif488TesterSignals[CHIP_488TEST_PIN_COUNT];

typedef struct Perif488Tester {
	CHIP_DECLARE_BASE

	SignalPool *			signal_pool;
	Perif488TesterSignals	signals;
} Perif488Tester;

// functions
Perif488Tester *perif_ieee488_tester_create(struct Simulator *sim, Perif488TesterSignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_PERIF_IEEE488_TESTER_H
