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

enum PerifDatassetteSignalAssignment {
	PIN_DS1530_GND			= CHIP_PIN_01,	// A-1
	PIN_DS1530_VCC			= CHIP_PIN_02,	// B-2
	PIN_DS1530_MOTOR		= CHIP_PIN_03,	// C-3
	PIN_DS1530_DATA_FROM_DS = CHIP_PIN_04,	// D-4
	PIN_DS1530_DATA_TO_DS	= CHIP_PIN_05,	// E-5
	PIN_DS1530_SENSE		= CHIP_PIN_06,	// F-6
};

#define CHIP_DS1530_PIN_COUNT 6
typedef Signal PerifDatassetteSignals[CHIP_DS1530_PIN_COUNT];

typedef struct TapData {
	int8_t *	raw;

	char *		file_path;

	int			version;
	uint8_t	*	data;		// pointer into raw, do not free
	uint8_t *	current;
	uint8_t *	end;

} TapData;

typedef struct PerifDatassette {
	CHIP_DECLARE_BASE

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

	int64_t					record_prev_tick;
	int32_t					record_count;

	TapData					tap;

} PerifDatassette;

// functions
PerifDatassette *perif_datassette_create(struct Simulator *sim, PerifDatassetteSignals signals);

void perif_datassette_key_pressed(PerifDatassette *datassette, PerifDatassetteKeys key);
void perif_datassette_load_tap_from_file(PerifDatassette *datassette, const char *filename);
void perif_datassette_load_tap_from_memory(PerifDatassette *datassette, const int8_t *data, size_t data_len);
void perif_datassette_new_tap(PerifDatassette *datassette, const char *filename);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_PERIF_DATASSETTE_1530_H
