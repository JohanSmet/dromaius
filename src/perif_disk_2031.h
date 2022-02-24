// perif_disk_2031_tester.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// A high-level emulation of the Commodore 2031 floppy drive
//	- partial implementation: just enough to make LOAD-command work

#ifndef DROMAIUS_PERIF_DISK_2031_H
#define DROMAIUS_PERIF_DISK_2031_H

#include "chip.h"
#include "img_d64.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
enum PerifDisk2031SignalAssignment {
	PERIF_FD2031_EOI_B = CHIP_PIN_01,
	PERIF_FD2031_DAV_B,
	PERIF_FD2031_NRFD_B,
	PERIF_FD2031_NDAC_B,
	PERIF_FD2031_ATN_B,
	PERIF_FD2031_SRQ_B,
	PERIF_FD2031_IFC_B,

	PERIF_FD2031_DIO0,
	PERIF_FD2031_DIO1,
	PERIF_FD2031_DIO2,
	PERIF_FD2031_DIO3,
	PERIF_FD2031_DIO4,
	PERIF_FD2031_DIO5,
	PERIF_FD2031_DIO6,
	PERIF_FD2031_DIO7,

	PERIF_FD2031_PIN_COUNT
};

typedef enum {
	FD2031_BUS_IDLE = 0,
	FD2031_BUS_ACCEPTOR_START,
	FD2031_BUS_ACCEPTOR_READY,
	FD2031_BUS_ACCEPTOR_DATA_AVAILABLE,
	FD2031_BUS_ACCEPTOR_DATA_TAKEN,
	FD2031_BUS_SOURCE_START,
	FD2031_BUS_SOURCE_READY,
	FD2031_BUS_SOURCE_DATA_VALID,
	FD2031_BUS_SOURCE_DATA_TAKEN,
} PerifDisk2031BusState;

typedef enum {
	FD2031_COMM_IDLE = 0,
	FD2031_COMM_LISTENING,
	FD2031_COMM_START_TALKING,
	FD2031_COMM_TALKING,
	FD2031_COMM_OPENING,
} PerifDisk2031CommState;

typedef struct PerifDisk2031Channel {
	bool		open;

	char		name[17];
	size_t		name_len;

	int8_t *	talk_data;
	ptrdiff_t	talk_available;
	uint16_t	talk_next_track_sector;
} PerifDisk2031Channel;

#define FD2031_MAX_CHANNELS	16

typedef Signal PerifDisk2031Signals[PERIF_FD2031_PIN_COUNT];

typedef struct PerifDisk2031 {
	CHIP_DECLARE_BASE

	// interface
	SignalPool *			signal_pool;
	PerifDisk2031Signals	signals;

	// signal groups
	SignalGroup				sg_dio;

	// data
	uint8_t					address;
	PerifDisk2031BusState	bus_state;
	PerifDisk2031CommState	comm_state;
	PerifDisk2031Channel	channels[FD2031_MAX_CHANNELS];
	size_t					active_channel;

	DiskImageD64 	 		d64_img;

	int64_t					next_wakeup;

	bool					output[PERIF_FD2031_PIN_COUNT];
	bool					force_output_low[PERIF_FD2031_PIN_COUNT];
} PerifDisk2031;

// functions
PerifDisk2031 *perif_fd2031_create(struct Simulator *sim, PerifDisk2031Signals signals);

void perif_fd2031_load_d64_from_file(PerifDisk2031 *disk, const char *filename);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_PERIF_DISK_2031_H
