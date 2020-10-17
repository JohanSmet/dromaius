// perif_datassette_1530.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a Commodore 1530 Datasette

#include "perif_datassette_1530.h"
#include "simulator.h"

#include "utils.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			datassette->signal_pool
#define SIGNAL_COLLECTION	datassette->signals
#define SIGNAL_CHIP_ID		datassette->id

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

typedef enum {
	STATE_IDLE			= 0,
	STATE_TAPE_LOADED	= 1,
	STATE_PLAYING		= 2,
	STATE_RECORDING		= 3,
	STATE_REWINDING		= 4,
	STATE_FFWDING		= 5
} PerifDatassetteState;

///////////////////////////////////////////////////////////////////////////////
//
// private functions
//

#define TAP_FILE_SIGNATURE "C64-TAPE-RAW"

static bool tap_load(const char *filename, TapData *tap) {
	assert(filename);
	assert(tap);

	if (!file_load_binary(filename, &tap->raw)) {
		return false;
	}

	// parse header
	if (!strncmp((const char *) tap->raw, TAP_FILE_SIGNATURE, sizeof(TAP_FILE_SIGNATURE))) {
		return false;
	}

	tap->version = tap->raw[0x0c];
	tap->data    = (uint8_t *) (tap->raw + 0x14);
	tap->current = tap->data;

	uint32_t tap_size = *((uint32_t *) (tap->raw + 0x10));

	if (arrlenu(tap->raw) != tap_size + 0x14) {
		return false;
	}

	return true;
}

static void tap_unload(TapData *tap) {
	assert(tap);
	arrfree(tap->raw);
	tap->raw = NULL;
}

static inline int64_t tap_current_interval_ps(TapData *tap) {

	static const int C64_PAL_FREQUENCY = 985248;
	int64_t interval_ps;

	if (tap->version == 1 && *tap->current == 0) {
		interval_ps = tap->current[0] + (tap->current[1] << 8) + (tap->current[2] << 16);
	} else {
		interval_ps = S_TO_PS((*tap->current * 8)) / C64_PAL_FREQUENCY;
	}

	return interval_ps;
}

static inline void tap_next_sample(TapData *tap) {
	tap->current += (tap->version == 1 && *tap->current == 0) ? 4 : 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

void perif_datassette_destroy(PerifDatassette *datassette);
void perif_datassette_register_dependencies(PerifDatassette *datassette);
void perif_datassette_process(PerifDatassette *datassette);

PerifDatassette *perif_datassette_create(struct Simulator *sim, PerifDatassetteSignals signals) {
	PerifDatassette *datassette = (PerifDatassette *) calloc(1, sizeof(PerifDatassette));

	// chip
	CHIP_SET_FUNCTIONS(datassette, perif_datassette_process, perif_datassette_destroy, perif_datassette_register_dependencies);

	// signals
	datassette->simulator = sim;
	datassette->signal_pool = sim->signal_pool;
	memcpy(&datassette->signals, &signals, sizeof(signals));

	SIGNAL_DEFINE_BOOL(gnd, 1, false);
	SIGNAL_DEFINE_BOOL(vcc, 1, false);
	SIGNAL_DEFINE(motor, 1);
	SIGNAL_DEFINE(data_from_ds, 1)
	SIGNAL_DEFINE(data_to_ds, 1);
	SIGNAL_DEFINE(sense, 1);

	// data
	datassette->sense_out = ACTLO_DEASSERT;
	datassette->data_out = true;
	datassette->valid_keys = DS_KEY_PLAY;
	datassette->idle_interval = simulator_interval_to_tick_count(datassette->simulator, MS_TO_PS(100));

	return datassette;
}

void perif_datassette_destroy(PerifDatassette *datassette) {
	assert(datassette);
	free(datassette);
}

void perif_datassette_register_dependencies(PerifDatassette *datassette) {
	assert(datassette);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(motor), datassette->id);
}

void perif_datassette_process(PerifDatassette *datassette) {
	assert(datassette);

	// sense signal
	SIGNAL_SET_BOOL(sense, datassette->sense_out);

	// don't do anything if motor isn't being driven
	if (!SIGNAL_BOOL(motor)) {
		// schedule sense signal check
		datassette->schedule_timestamp = datassette->simulator->current_tick + datassette->idle_interval;
		return;
	}

	// FIXME: assumes 'playing' state
	if (datassette->state == STATE_PLAYING && datassette->tick_next_transition <= datassette->simulator->current_tick) {
		if (datassette->data_out) {
			// next sample
			int64_t interval_ps = tap_current_interval_ps(&datassette->tap);
			datassette->sample_interval = simulator_interval_to_tick_count(datassette->simulator, interval_ps) / 2;
			datassette->offset_ps += interval_ps;
			datassette->tick_next_transition = datassette->simulator->current_tick + datassette->sample_interval;
			datassette->data_out = false;
		} else {
			// 2nd half of current sample
			datassette->tick_next_transition = datassette->simulator->current_tick + datassette->sample_interval;
			datassette->data_out = true;
			tap_next_sample(&datassette->tap);
		}

		SIGNAL_SET_BOOL(data_from_ds, datassette->data_out);
	}

	datassette->schedule_timestamp = datassette->tick_next_transition;
}

void perif_datassette_key_pressed(PerifDatassette *datassette, PerifDatassetteKeys key) {
	assert(datassette);

	if ((datassette->valid_keys & key) == 0) {
		return;
	}

	switch (key) {
		case DS_KEY_RECORD:
			datassette->valid_keys = DS_KEY_STOP;
			datassette->state = STATE_RECORDING;
			datassette->sense_out = ACTLO_ASSERT;
			break;
		case DS_KEY_PLAY :
			datassette->valid_keys = DS_KEY_STOP;
			datassette->state = STATE_PLAYING;
			datassette->sense_out = ACTLO_ASSERT;
			break;
		case DS_KEY_REWIND :
			datassette->valid_keys = DS_KEY_STOP;
			datassette->state = STATE_REWINDING;
			datassette->sense_out = ACTLO_ASSERT;
			break;
		case DS_KEY_FFWD :
			datassette->valid_keys = DS_KEY_STOP;
			datassette->state = STATE_FFWDING;
			datassette->sense_out = ACTLO_ASSERT;
			break;
		case DS_KEY_STOP :
			datassette->valid_keys = DS_KEY_RECORD | DS_KEY_PLAY | DS_KEY_REWIND | DS_KEY_FFWD | DS_KEY_EJECT;
			datassette->state = STATE_TAPE_LOADED;
			datassette->sense_out = ACTLO_DEASSERT;
			break;
		case DS_KEY_EJECT :
			datassette->valid_keys = 0;
			datassette->state = STATE_IDLE;
			datassette->sense_out = ACTLO_DEASSERT;
			tap_unload(&datassette->tap);
			break;
	}
}

void perif_datassette_load(PerifDatassette *datassette, const char *filename) {
	assert(datassette);
	assert(filename);

	if (tap_load(filename, &datassette->tap)) {
		datassette->state = STATE_TAPE_LOADED;
		datassette->valid_keys = DS_KEY_RECORD | DS_KEY_PLAY | DS_KEY_REWIND | DS_KEY_FFWD | DS_KEY_EJECT;
	}
}
