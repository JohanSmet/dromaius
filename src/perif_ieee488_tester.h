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

typedef enum {
	IEEE488_BUS_IDLE = 0,
	IEEE488_BUS_ACCEPTOR_START,
	IEEE488_BUS_ACCEPTOR_READY,
	IEEE488_BUS_ACCEPTOR_DATA_AVAILABLE,
	IEEE488_BUS_ACCEPTOR_DATA_TAKEN,
	IEEE488_BUS_SOURCE_START,
	IEEE488_BUS_SOURCE_READY,
	IEEE488_BUS_SOURCE_DATA_VALID,
	IEEE488_BUS_SOURCE_DATA_TAKEN,
} Perif488TesterBusState;

typedef enum {
	IEEE488_COMM_IDLE = 0,
	IEEE488_COMM_LISTENING,
	IEEE488_COMM_START_TALKING,
	IEEE488_COMM_TALKING,
	IEEE488_COMM_OPENING,
} Perif488TesterCommState;

typedef struct Perif488Channel {
	bool	open;

	char	name[17];
	size_t	name_len;

	uint8_t *talk_data;
	size_t  talk_available;
} Perif488Channel;

#define PERIF488_MAX_CHANNELS	16

typedef Signal Perif488TesterSignals[CHIP_488TEST_PIN_COUNT];

typedef struct Perif488Tester {
	CHIP_DECLARE_BASE

	// interface
	SignalPool *			signal_pool;
	Perif488TesterSignals	signals;

	// signal groups
	SignalGroup				sg_dio;

	// data
	uint8_t					address;
	Perif488TesterBusState	bus_state;
	Perif488TesterCommState comm_state;
	Perif488Channel			channels[PERIF488_MAX_CHANNELS];
	size_t					active_channel;

	bool					output[CHIP_488TEST_PIN_COUNT];
	bool					force_output_low[CHIP_488TEST_PIN_COUNT];
} Perif488Tester;

// functions
Perif488Tester *perif_ieee488_tester_create(struct Simulator *sim, Perif488TesterSignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_PERIF_IEEE488_TESTER_H
