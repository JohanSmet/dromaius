// perif_ieee488_tester.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// A periphiral to test/monitor the IEEE-488 bus

#include "perif_ieee488_tester.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>

//#define DMS_LOG_TRACE
#define LOG_SIMULATOR		chip->simulator
#include "log.h"

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

static uint8_t TEST_DATA[] = {
	0x01, 0x04, 0x0f, 0x04, 0x0a, 0x00, 0x99, 0x20, 0x22, 0x48, 0x45, 0x4c, 0x4c, 0x4f, 0x22, 0x00, 0x00, 0x00
};

#define IEEE_MASK_COMMAND	0xf0
#define IEEE_MASK_ADDRESS	0x0f

#define CMD_LISTEN		0x20
#define CMD_UNLISTEN	0x30
#define CMD_TALK		0x40
#define CMD_UNTALK		0x50
#define CMD_SECOND		0x60
#define CMD_CLOSE		0xe0
#define CMD_OPEN		0xf0

static void perif_ieee488_tester_destroy(Perif488Tester *chip);
static void perif_ieee488_tester_process(Perif488Tester *chip);

////////////////////////////////////////////////////////////////////////////////
//
// private functions
//

static inline char petscii_to_ascii(int pet) {
	static const char LUT[256] = {
		[0x20] = ' ',	[0x21] = '!',	[0x22] = '"',	[0x23] = '#',
		[0x24] = '$',	[0x25] = '%',	[0x26] = '&',	[0x27] = '\'',
		[0x28] = '(',	[0x29] = ')',	[0x2A] = '*',	[0x2B] = '+',
		[0x2C] = ',',	[0x2D] = '-',	[0x2E] = '.',	[0x2F] = '/',
		[0x30] = '0',	[0x31] = '1',	[0x32] = '2',	[0x33] = '3',
		[0x34] = '4',	[0x35] = '5',	[0x36] = '6',	[0x37] = '7',
		[0x38] = '8',	[0x39] = '9',	[0x3A] = ':',	[0x3B] = ';',
		[0x3C] = '<',	[0x3D] = '=',	[0x3E] = '>',	[0x3F] = '?',
		[0x40] = '@',	[0x41] = 'A',	[0x42] = 'B',	[0x43] = 'C',
		[0x44] = 'D',	[0x45] = 'E',	[0x46] = 'F',	[0x47] = 'G',
		[0x48] = 'H',	[0x49] = 'I',	[0x4A] = 'J',	[0x4B] = 'K',
		[0x4C] = 'L',	[0x4D] = 'M',	[0x4E] = 'N',	[0x4F] = 'O',
		[0x50] = 'P',	[0x51] = 'Q',	[0x52] = 'R',	[0x53] = 'S',
		[0x54] = 'T',	[0x55] = 'U',	[0x56] = 'V',	[0x57] = 'W',
		[0x58] = 'X',	[0x59] = 'Y',	[0x5A] = 'Z',	[0x5B] = '[',
		[0x5D] = ']',
	};

	return LUT[pet];
}

static inline uint8_t ieee488_read_data(Perif488Tester *chip) {
	return ~SIGNAL_GROUP_READ_U8(dio);
}

static void ieee488_handle_command_in(Perif488Tester *chip) {
	assert(chip);

	uint8_t data = ieee488_read_data(chip);
	uint8_t command = data & IEEE_MASK_COMMAND;
	uint8_t address = data & IEEE_MASK_ADDRESS;

	switch (command) {
		case CMD_LISTEN:
			LOG_TRACE("IEEE488 [%.2x] - LISTEN %d", data, address);
			if (chip->address == address) {
				chip->comm_state = IEEE488_COMM_LISTENING;
				chip->bus_state = IEEE488_BUS_ACCEPTOR_READY;
			}
			break;
		case CMD_UNLISTEN:
			LOG_TRACE("IEEE488 [%.2x] - UNLISTEN", data);
			chip->comm_state = IEEE488_COMM_IDLE;
			chip->bus_state = IEEE488_BUS_IDLE;
			break;
		case CMD_TALK:
			LOG_TRACE("IEEE488 [%.2x] - TALK %d", data, address);
			if (chip->address == address) {
				chip->comm_state = IEEE488_COMM_START_TALKING;
			}
			break;
		case CMD_UNTALK:
			LOG_TRACE("IEEE488 [%.2x] - UNTALK", data);
			chip->comm_state = IEEE488_COMM_IDLE;
			chip->bus_state = IEEE488_BUS_IDLE;
			break;
		case CMD_SECOND:
			LOG_TRACE("IEEE488 [%.2x] - CHANNEL %d", data, address);
			if (chip->comm_state == IEEE488_COMM_LISTENING) {
				chip->active_channel = address;
			} else if (chip->comm_state == IEEE488_COMM_START_TALKING) {
				chip->active_channel = address;
				chip->channels[address].talk_data = TEST_DATA;
				chip->channels[address].talk_available = sizeof(TEST_DATA);
				chip->comm_state = IEEE488_COMM_TALKING;
			}
			break;
		case CMD_CLOSE:
			LOG_TRACE("IEEE488 [%.2x] - CLOSE", data);
			if (chip->comm_state == IEEE488_COMM_LISTENING) {
				chip->channels[chip->active_channel].open = false;
			}
			break;
		case CMD_OPEN:
			LOG_TRACE("IEEE488 [%.2x] - OPEN %d", data, address);
			if (chip->comm_state == IEEE488_COMM_LISTENING) {
				chip->active_channel = address;
				chip->comm_state = IEEE488_COMM_OPENING;
				chip->channels[chip->active_channel].open = true;
				memset(chip->channels[chip->active_channel].name, '\0', 16);
				chip->channels[chip->active_channel].name_len = 0;
			}
			break;
		default:
			LOG_TRACE("IEEE488 [%.2x] - UNKNOWN COMMAND", data);
	}


}

static void ieee488_handle_data_in(Perif488Tester *chip) {
	assert(chip);

	uint8_t data = ieee488_read_data(chip);
	LOG_TRACE("IEEE488 [%.2x] - DATA IN (%c)", data, petscii_to_ascii(data));

	if (chip->comm_state == IEEE488_COMM_OPENING) {
		size_t idx = chip->channels[chip->active_channel].name_len++;
		chip->channels[chip->active_channel].name[idx] = petscii_to_ascii(data);
	}
}

static void ieee488_reset(Perif488Tester *chip) {
	assert(chip);

	chip->bus_state = IEEE488_BUS_IDLE;
	chip->comm_state = IEEE488_COMM_IDLE;
	chip->active_channel = 0;

	for (size_t idx = 0; idx < PERIF488_MAX_CHANNELS; ++idx) {
		memset(chip->channels[idx].name, 0, 16);
		chip->channels[idx].name_len = 0;
		chip->channels[idx].open = false;
		chip->channels[idx].talk_data = NULL;
		chip->channels[idx].talk_available = false;
	}

	for (size_t idx = 0; idx < CHIP_488TEST_PIN_COUNT; ++idx) {
		chip->output[idx] = false;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

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

	chip->sg_dio = signal_group_create();
	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(DIO0 + i, dio);
	}

	// data
	chip->address = 8;

	return chip;
}

static void perif_ieee488_tester_destroy(Perif488Tester *chip) {
	assert(chip);
	free(chip);
}

static void perif_ieee488_tester_process(Perif488Tester *chip) {
	assert(chip);

	if (SIGNAL_CHANGED(IFC_B) && ACTLO_ASSERTED(SIGNAL_READ(IFC_B))) {
		LOG_TRACE("IEEE488 - %s", "RESET");
		ieee488_reset(chip);
		return;
	}

	if (SIGNAL_CHANGED(ATN_B) && ACTLO_ASSERTED(SIGNAL_READ(ATN_B))) {
		// controller wants to send a command, be a good device and start listening right away.
		chip->bus_state = IEEE488_BUS_ACCEPTOR_START;
	}

	switch (chip->bus_state) {
		case IEEE488_BUS_IDLE:
			chip->output[SIGNAL_ENUM(NRFD_B)] = false;
			chip->output[SIGNAL_ENUM(NDAC_B)] = false;
			break;

		case IEEE488_BUS_ACCEPTOR_START:
			chip->output[SIGNAL_ENUM(NRFD_B)] = true;
			chip->output[SIGNAL_ENUM(NDAC_B)] = true;
			chip->bus_state = IEEE488_BUS_ACCEPTOR_READY;
			break;

		case IEEE488_BUS_ACCEPTOR_READY:
			// signal we're ready to accept data
			chip->output[SIGNAL_ENUM(NRFD_B)] = false;

			if (ACTLO_ASSERTED(SIGNAL_READ(DAV_B))) {
				chip->bus_state = IEEE488_BUS_ACCEPTOR_DATA_AVAILABLE;
			}
			break;

		case IEEE488_BUS_ACCEPTOR_DATA_AVAILABLE:
			chip->output[SIGNAL_ENUM(NRFD_B)] = true;
			if (ACTLO_ASSERTED(SIGNAL_READ(ATN_B))) {
				ieee488_handle_command_in(chip);
			} else {
				ieee488_handle_data_in(chip);
			}
			chip->output[SIGNAL_ENUM(NDAC_B)] = false;
			chip->bus_state = IEEE488_BUS_ACCEPTOR_DATA_TAKEN;
			break;

		case IEEE488_BUS_ACCEPTOR_DATA_TAKEN:
			if (!ACTLO_ASSERTED(SIGNAL_READ(DAV_B))) {
				chip->output[SIGNAL_ENUM(NDAC_B)] = true;
				if (chip->comm_state == IEEE488_COMM_TALKING) {
					chip->bus_state = IEEE488_BUS_SOURCE_START;
				} else if (chip->comm_state != IEEE488_COMM_IDLE) {
					chip->bus_state = IEEE488_BUS_ACCEPTOR_READY;
				} else {
					chip->bus_state = IEEE488_BUS_IDLE;
				}
			}
			break;

		case IEEE488_BUS_SOURCE_START:
			chip->output[SIGNAL_ENUM(DAV_B)] = false;
			chip->output[SIGNAL_ENUM(NRFD_B)] = false;
			chip->output[SIGNAL_ENUM(NDAC_B)] = false;
			if (ACTLO_ASSERTED(SIGNAL_READ(ATN_B))) {
				// wait until controller has released ATN
				chip->bus_state = IEEE488_BUS_SOURCE_START;
			} else if (!ACTLO_ASSERTED(SIGNAL_READ(NRFD_B)) && !ACTLO_ASSERTED(SIGNAL_READ(NDAC_B))) {
				// error
				chip->bus_state = IEEE488_BUS_IDLE;
			} else {
				chip->bus_state = IEEE488_BUS_SOURCE_READY;
			}
			break;

		case IEEE488_BUS_SOURCE_READY: {
			uint8_t data = (chip->channels[chip->active_channel].talk_available > 0) ? *chip->channels[chip->active_channel].talk_data : 0;

			// SIGNAL_GROUP_WRITE(dio, data);
			chip->output[SIGNAL_ENUM(DIO0)] = data & 0b00000001;
			chip->output[SIGNAL_ENUM(DIO1)] = data & 0b00000010;
			chip->output[SIGNAL_ENUM(DIO2)] = data & 0b00000100;
			chip->output[SIGNAL_ENUM(DIO3)] = data & 0b00001000;
			chip->output[SIGNAL_ENUM(DIO4)] = data & 0b00010000;
			chip->output[SIGNAL_ENUM(DIO5)] = data & 0b00100000;
			chip->output[SIGNAL_ENUM(DIO6)] = data & 0b01000000;
			chip->output[SIGNAL_ENUM(DIO7)] = data & 0b10000000;

			if (chip->channels[chip->active_channel].talk_available <= 1) {
				chip->output[SIGNAL_ENUM(EOI_B)] = true;
			}
			if (!ACTLO_ASSERTED(SIGNAL_READ(NRFD_B))) {
				chip->bus_state = IEEE488_BUS_SOURCE_DATA_VALID;
			}
			break;
		}

		case IEEE488_BUS_SOURCE_DATA_VALID:
			chip->output[SIGNAL_ENUM(DAV_B)] = true;
			if (!ACTLO_ASSERTED(SIGNAL_READ(NDAC_B))) {
				chip->bus_state = IEEE488_BUS_SOURCE_DATA_TAKEN;
			}
			break;

		case IEEE488_BUS_SOURCE_DATA_TAKEN:
			chip->output[SIGNAL_ENUM(DAV_B)] = false;
			chip->output[SIGNAL_ENUM(EOI_B)] = false;
			chip->output[SIGNAL_ENUM(DIO0)] = false;
			chip->output[SIGNAL_ENUM(DIO1)] = false;
			chip->output[SIGNAL_ENUM(DIO2)] = false;
			chip->output[SIGNAL_ENUM(DIO3)] = false;
			chip->output[SIGNAL_ENUM(DIO4)] = false;
			chip->output[SIGNAL_ENUM(DIO5)] = false;
			chip->output[SIGNAL_ENUM(DIO6)] = false;
			chip->output[SIGNAL_ENUM(DIO7)] = false;
			chip->channels[chip->active_channel].talk_data += 1;
			chip->channels[chip->active_channel].talk_available -= 1;;
			if (chip->channels[chip->active_channel].talk_available > 0) {
				chip->bus_state = IEEE488_BUS_SOURCE_READY;
			} else {
				chip->bus_state = IEEE488_BUS_IDLE;
				chip->comm_state = IEEE488_COMM_IDLE;
			}
			break;

		default:
			assert(false);
	}

	// manually forced low (true) lines
	for (int i = 0; i < CHIP_488TEST_PIN_COUNT; ++i) {
		if (chip->output[i] || chip->force_output_low[i]) {
			signal_write(SIGNAL_POOL, chip->signals[i], false);
		} else {
			signal_clear_writer(SIGNAL_POOL, chip->signals[i]);
		}
	}

	// schedule wakeup to check for 'commands' from the UI-panel
	chip->schedule_timestamp = chip->simulator->current_tick + simulator_interval_to_tick_count(chip->simulator, MS_TO_PS(100));
}
