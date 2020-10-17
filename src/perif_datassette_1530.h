// perif_datassette_1530.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a Commodore 1530 Datasette

#ifndef DROMAIUS_PERIF_DATASSETTE_1530_H
#define DROMAIUS_PERIF_DATASSETTE_1530_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef enum {
	DS_KEY_RECORD	= 1 << 0,
	DS_KEY_PLAY		= 1 << 1,
	DS_KEY_REWIND	= 1 << 2,
	DS_KEY_FFWD		= 1 << 3,
	DS_KEY_STOP		= 1 << 4,
	DS_KEY_EJECT	= 1 << 5
} PerifDatassetteKeys;

typedef struct PerifDatassetteSignals {
	Signal	gnd;			// A-1
	Signal	vcc;			// B-2
	Signal	motor;			// C-3
	Signal	data_from_ds;	// D-4
	Signal	data_to_ds;		// E-5
	Signal	sense;			// F-6
} PerifDatassetteSignals;

typedef struct TapData {
	int8_t *	raw;

	int			version;
	uint8_t	*	data;		// pointer into raw, do not free
	uint8_t *	current;

} TapData;

typedef struct PerifDatassette {
	CHIP_DECLARE_FUNCTIONS

	SignalPool *			signal_pool;
	PerifDatassetteSignals	signals;

	int						state;
	uint32_t				valid_keys;

	bool					sense_out;
	bool					data_out;

	int64_t					idle_interval;
	int64_t					sample_interval;
	int64_t					tick_next_transition;
	int64_t					offset_ps;

	TapData					tap;

} PerifDatassette;

// functions
PerifDatassette *perif_datassette_create(struct Simulator *sim, PerifDatassetteSignals signals);

void perif_datassette_key_pressed(PerifDatassette *datassette, PerifDatassetteKeys key);
void perif_datassette_load(PerifDatassette *datassette, const char *filename);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_PERIF_DATASSETTE_1530_H
