// perif_disk_2031.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// A periphiral to test/monitor the IEEE-488 bus

#include "perif_disk_2031.h"
#include "simulator.h"
#include "utils.h"
#include "img_d64.h"

#include <assert.h>
#include <stdlib.h>

#define DMS_LOG_TRACE
#define LOG_SIMULATOR		disk->simulator
#include "log.h"

#define SIGNAL_PREFIX		PERIF_FD2031_
#define SIGNAL_OWNER		disk

static uint8_t PerifDisk2031_PinTypes[PERIF_FD2031_PIN_COUNT] = {
	[SIGNAL_ENUM(EOI_B) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[SIGNAL_ENUM(DAV_B) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[SIGNAL_ENUM(NRFD_B)] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[SIGNAL_ENUM(NDAC_B)] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[SIGNAL_ENUM(ATN_B) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[SIGNAL_ENUM(SRQ_B) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[SIGNAL_ENUM(IFC_B) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,

	[SIGNAL_ENUM(DIO0) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[SIGNAL_ENUM(DIO1) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[SIGNAL_ENUM(DIO2) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[SIGNAL_ENUM(DIO3) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[SIGNAL_ENUM(DIO4) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[SIGNAL_ENUM(DIO5) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[SIGNAL_ENUM(DIO6) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[SIGNAL_ENUM(DIO7) ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
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

static void perif_fd2031_destroy(PerifDisk2031 *disk);
static void perif_fd2031_process(PerifDisk2031 *disk);

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
		[0x5D] = ']',	[0xA0] = ' '
	};

	return LUT[pet];
}

static void prv2031_load_channel_data(PerifDisk2031 *disk) {

	PerifDisk2031Channel *channel = &disk->channels[disk->active_channel];

	if (!img_d64_is_valid(&disk->d64_img)) {
		channel->talk_available = 0;
		channel->talk_data = NULL;
		channel->talk_next_track_sector = 0;
	} else if (channel->name_len == 1 && channel->name[0] == '$') {
		channel->talk_available = img_d64_basic_directory_list(&disk->d64_img, &channel->talk_data);
		channel->talk_next_track_sector = 0;
	} else {
		uint16_t file_track_sector = img_d64_file_start_track_sector(&disk->d64_img, (uint8_t *) channel->name, channel->name_len);
		channel->talk_available = img_d64_file_block(
										&disk->d64_img, file_track_sector,
										(uint8_t **) &channel->talk_data, &channel->talk_next_track_sector);
	}
}

static inline uint8_t prv2031_read_data(PerifDisk2031 *disk) {
	return ~SIGNAL_GROUP_READ_U8(dio);
}

static void prv2031_handle_command_in(PerifDisk2031 *disk) {
	assert(disk);

	uint8_t data = prv2031_read_data(disk);
	uint8_t command = data & IEEE_MASK_COMMAND;
	uint8_t address = data & IEEE_MASK_ADDRESS;

	switch (command) {
		case CMD_LISTEN:
			LOG_TRACE("FD2031 [%.2x] - LISTEN %d", data, address);
			if (disk->address == address) {
				disk->comm_state = FD2031_COMM_LISTENING;
				disk->bus_state = FD2031_BUS_ACCEPTOR_READY;
			}
			break;
		case CMD_UNLISTEN:
			LOG_TRACE("FD2031 [%.2x] - UNLISTEN", data);
			disk->comm_state = FD2031_COMM_IDLE;
			disk->bus_state = FD2031_BUS_IDLE;
			break;
		case CMD_TALK:
			LOG_TRACE("FD2031 [%.2x] - TALK %d", data, address);
			if (disk->address == address) {
				disk->comm_state = FD2031_COMM_START_TALKING;
			}
			break;
		case CMD_UNTALK:
			LOG_TRACE("FD2031 [%.2x] - UNTALK", data);
			disk->comm_state = FD2031_COMM_IDLE;
			disk->bus_state = FD2031_BUS_IDLE;
			break;
		case CMD_SECOND:
			LOG_TRACE("FD2031 [%.2x] - CHANNEL %d", data, address);
			if (disk->comm_state == FD2031_COMM_LISTENING) {
				disk->active_channel = address;
			} else if (disk->comm_state == FD2031_COMM_START_TALKING) {
				disk->active_channel = address;
				prv2031_load_channel_data(disk);
				disk->comm_state = FD2031_COMM_TALKING;
			}
			break;
		case CMD_CLOSE:
			LOG_TRACE("FD2031 [%.2x] - CLOSE", data);
			if (disk->comm_state == FD2031_COMM_LISTENING) {
				disk->channels[disk->active_channel].open = false;
			}
			break;
		case CMD_OPEN:
			LOG_TRACE("FD2031 [%.2x] - OPEN %d", data, address);
			if (disk->comm_state == FD2031_COMM_LISTENING) {
				disk->active_channel = address;
				disk->comm_state = FD2031_COMM_OPENING;
				disk->channels[disk->active_channel].open = true;
				memset(disk->channels[disk->active_channel].name, '\0', 16);
				disk->channels[disk->active_channel].name_len = 0;
			}
			break;
		default:
			LOG_TRACE("FD2031 [%.2x] - UNKNOWN COMMAND", data);
	}


}

static void prv2031_handle_data_in(PerifDisk2031 *disk) {
	assert(disk);

	uint8_t data = prv2031_read_data(disk);
	LOG_TRACE("FD2031 [%.2x] - DATA IN (%c)", data, petscii_to_ascii(data));

	if (disk->comm_state == FD2031_COMM_OPENING) {
		size_t idx = disk->channels[disk->active_channel].name_len++;
		disk->channels[disk->active_channel].name[idx] = petscii_to_ascii(data);
	}
}

static void prv2031_reset(PerifDisk2031 *disk) {
	assert(disk);

	disk->bus_state = FD2031_BUS_IDLE;
	disk->comm_state = FD2031_COMM_IDLE;
	disk->active_channel = 0;

	for (size_t idx = 0; idx < FD2031_MAX_CHANNELS; ++idx) {
		memset(disk->channels[idx].name, 0, 16);
		disk->channels[idx].name_len = 0;
		disk->channels[idx].open = false;
		disk->channels[idx].talk_data = NULL;
		disk->channels[idx].talk_available = false;
	}

	for (size_t idx = 0; idx < PERIF_FD2031_PIN_COUNT; ++idx) {
		disk->output[idx] = false;
	}
}

static inline void prv2031_change_bus_state(PerifDisk2031 *disk, PerifDisk2031BusState new_state) {
	disk->bus_state = new_state;
	disk->schedule_timestamp = disk->simulator->current_tick + simulator_interval_to_tick_count(disk->simulator, MS_TO_PS(100));
}

////////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

PerifDisk2031 *perif_fd2031_create(Simulator *sim, PerifDisk2031Signals signals) {
	PerifDisk2031 *disk = (PerifDisk2031 *) calloc(1, sizeof(PerifDisk2031));

	// disk
	CHIP_SET_FUNCTIONS(disk, perif_fd2031_process, perif_fd2031_destroy);
	CHIP_SET_VARIABLES(disk, sim, disk->signals, PerifDisk2031_PinTypes, PERIF_FD2031_PIN_COUNT);

	// signals
	disk->signal_pool = sim->signal_pool;
	memcpy(disk->signals, signals, sizeof(PerifDisk2031Signals));

	SIGNAL_DEFINE_DEFAULT(EOI_B, true);
	SIGNAL_DEFINE_DEFAULT(DAV_B, true);
	SIGNAL_DEFINE_DEFAULT(NRFD_B, true);
	SIGNAL_DEFINE_DEFAULT(NDAC_B, true);
	SIGNAL_DEFINE_DEFAULT(ATN_B, true);
	SIGNAL_DEFINE_DEFAULT(SRQ_B, true);
	SIGNAL_DEFINE_DEFAULT(IFC_B, true);

	disk->sg_dio = signal_group_create();
	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(DIO0 + i, dio);
	}

	// data
	disk->address = 8;

	return disk;
}

static void perif_fd2031_destroy(PerifDisk2031 *disk) {
	assert(disk);
	free(disk);
}

static void perif_fd2031_process(PerifDisk2031 *disk) {
	assert(disk);

	if (SIGNAL_CHANGED(IFC_B) && ACTLO_ASSERTED(SIGNAL_READ(IFC_B))) {
		LOG_TRACE("FD2031 - %s", "RESET");
		prv2031_reset(disk);
		return;
	}

	if (SIGNAL_CHANGED(ATN_B) && ACTLO_ASSERTED(SIGNAL_READ(ATN_B))) {
		// controller wants to send a command, be a good device and start listening right away.
		disk->bus_state = FD2031_BUS_ACCEPTOR_START;
	}

	switch (disk->bus_state) {
		case FD2031_BUS_IDLE:
			disk->output[SIGNAL_ENUM(NRFD_B)] = false;
			disk->output[SIGNAL_ENUM(NDAC_B)] = false;
			break;

		case FD2031_BUS_ACCEPTOR_START:
			disk->output[SIGNAL_ENUM(NRFD_B)] = true;
			disk->output[SIGNAL_ENUM(NDAC_B)] = true;
			prv2031_change_bus_state(disk, FD2031_BUS_ACCEPTOR_READY);
			break;

		case FD2031_BUS_ACCEPTOR_READY:
			// signal we're ready to accept data
			disk->output[SIGNAL_ENUM(NRFD_B)] = false;

			if (ACTLO_ASSERTED(SIGNAL_READ(DAV_B))) {
				prv2031_change_bus_state(disk, FD2031_BUS_ACCEPTOR_DATA_AVAILABLE);
			}
			break;

		case FD2031_BUS_ACCEPTOR_DATA_AVAILABLE:
			disk->output[SIGNAL_ENUM(NRFD_B)] = true;
			if (ACTLO_ASSERTED(SIGNAL_READ(ATN_B))) {
				prv2031_handle_command_in(disk);
			} else {
				prv2031_handle_data_in(disk);
			}
			disk->output[SIGNAL_ENUM(NDAC_B)] = false;
			prv2031_change_bus_state(disk, FD2031_BUS_ACCEPTOR_DATA_TAKEN);
			break;

		case FD2031_BUS_ACCEPTOR_DATA_TAKEN:
			if (!ACTLO_ASSERTED(SIGNAL_READ(DAV_B))) {
				disk->output[SIGNAL_ENUM(NDAC_B)] = true;
				if (disk->comm_state == FD2031_COMM_TALKING) {
					prv2031_change_bus_state(disk, FD2031_BUS_SOURCE_START);
				} else if (disk->comm_state != FD2031_COMM_IDLE) {
					prv2031_change_bus_state(disk, FD2031_BUS_ACCEPTOR_READY);
				} else {
					prv2031_change_bus_state(disk, FD2031_BUS_IDLE);
				}
			}
			break;

		case FD2031_BUS_SOURCE_START:
			disk->output[SIGNAL_ENUM(DAV_B)] = false;
			disk->output[SIGNAL_ENUM(NRFD_B)] = false;
			disk->output[SIGNAL_ENUM(NDAC_B)] = false;
			if (ACTLO_ASSERTED(SIGNAL_READ(ATN_B))) {
				// wait until controller has released ATN
				prv2031_change_bus_state(disk, FD2031_BUS_SOURCE_START);
			} else if (!ACTLO_ASSERTED(SIGNAL_READ(NRFD_B)) && !ACTLO_ASSERTED(SIGNAL_READ(NDAC_B))) {
				// error
				prv2031_change_bus_state(disk, FD2031_BUS_IDLE);
			} else {
				prv2031_change_bus_state(disk, FD2031_BUS_SOURCE_READY);
			}
			break;

		case FD2031_BUS_SOURCE_READY: {
			if (disk->channels[disk->active_channel].talk_available <= 0) {
				// no data available - no disk loaded or file not found
				prv2031_change_bus_state(disk, FD2031_BUS_IDLE);
				disk->comm_state = FD2031_COMM_IDLE;
				break;
			}

			int8_t data = *disk->channels[disk->active_channel].talk_data;

			// SIGNAL_GROUP_WRITE(dio, data);
			disk->output[SIGNAL_ENUM(DIO0)] = data & 0b00000001;
			disk->output[SIGNAL_ENUM(DIO1)] = data & 0b00000010;
			disk->output[SIGNAL_ENUM(DIO2)] = data & 0b00000100;
			disk->output[SIGNAL_ENUM(DIO3)] = data & 0b00001000;
			disk->output[SIGNAL_ENUM(DIO4)] = data & 0b00010000;
			disk->output[SIGNAL_ENUM(DIO5)] = data & 0b00100000;
			disk->output[SIGNAL_ENUM(DIO6)] = data & 0b01000000;
			disk->output[SIGNAL_ENUM(DIO7)] = data & 0b10000000;

			if (disk->channels[disk->active_channel].talk_available <= 1 && disk->channels[disk->active_channel].talk_next_track_sector == 0) {
				disk->output[SIGNAL_ENUM(EOI_B)] = true;
			}
			if (!ACTLO_ASSERTED(SIGNAL_READ(NRFD_B))) {
				prv2031_change_bus_state(disk, FD2031_BUS_SOURCE_DATA_VALID);
			}
			break;
		}

		case FD2031_BUS_SOURCE_DATA_VALID:
			disk->output[SIGNAL_ENUM(DAV_B)] = true;
			if (!ACTLO_ASSERTED(SIGNAL_READ(NDAC_B))) {
				prv2031_change_bus_state(disk, FD2031_BUS_SOURCE_DATA_TAKEN);
			}
			break;

		case FD2031_BUS_SOURCE_DATA_TAKEN: {
			PerifDisk2031Channel *channel = &disk->channels[disk->active_channel];
			disk->output[SIGNAL_ENUM(DAV_B)] = false;
			disk->output[SIGNAL_ENUM(EOI_B)] = false;
			disk->output[SIGNAL_ENUM(DIO0)] = false;
			disk->output[SIGNAL_ENUM(DIO1)] = false;
			disk->output[SIGNAL_ENUM(DIO2)] = false;
			disk->output[SIGNAL_ENUM(DIO3)] = false;
			disk->output[SIGNAL_ENUM(DIO4)] = false;
			disk->output[SIGNAL_ENUM(DIO5)] = false;
			disk->output[SIGNAL_ENUM(DIO6)] = false;
			disk->output[SIGNAL_ENUM(DIO7)] = false;
			channel->talk_data += 1;
			channel->talk_available -= 1;
			if (channel->talk_available > 0) {
				prv2031_change_bus_state(disk, FD2031_BUS_SOURCE_READY);
			} else if (channel->talk_next_track_sector > 0) {
				channel->talk_available = img_d64_file_block(
											&disk->d64_img, channel->talk_next_track_sector,
											(uint8_t **) &channel->talk_data, &channel->talk_next_track_sector);
				prv2031_change_bus_state(disk, FD2031_BUS_SOURCE_READY);
			} else {
				prv2031_change_bus_state(disk, FD2031_BUS_IDLE);
				disk->comm_state = FD2031_COMM_IDLE;
			}
			break;
		}

		default:
			assert(false);
	}

	// manually forced low (true) lines
	for (int i = 0; i < PERIF_FD2031_PIN_COUNT; ++i) {
		if (disk->output[i] || disk->force_output_low[i]) {
			signal_write(SIGNAL_POOL, disk->signals[i], false);
		} else {
			signal_clear_writer(SIGNAL_POOL, disk->signals[i]);
		}
	}

	// schedule wakeup to check for 'commands' from the UI-panel
	if (disk->simulator->current_tick >= disk->next_wakeup) {
		disk->next_wakeup = disk->simulator->current_tick + simulator_interval_to_tick_count(disk->simulator, MS_TO_PS(100));
		disk->schedule_timestamp = disk->next_wakeup;
	}
}

void perif_fd2031_load_d64_from_file(PerifDisk2031 *tester, const char *filename) {
	assert(tester);
	assert(filename);

	int8_t *raw = NULL;
	if (!file_load_binary(filename, &raw)) {
	}

	img_d64_parse_memory(raw, &tester->d64_img);
}

