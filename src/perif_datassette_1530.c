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
#define TAP_REW_FFWD_SAMPLES	1000

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

	tap->end = tap->data + tap_size;
	tap->file_path = strdup(filename);

	return true;
}

static bool tap_new(const char *filename, TapData *tap) {

	arrsetlen(tap->raw, 0x14);

	// header
	tap->version = 0x01;
	memcpy((char *) tap->raw, TAP_FILE_SIGNATURE, sizeof(TAP_FILE_SIGNATURE));
	tap->raw[0x0c] = (int8_t) tap->version;


	tap->data    = (uint8_t *) (tap->raw + 0x14);
	tap->current = tap->data;
	tap->end	 = tap->current;
	tap->file_path = strdup(filename);

	return true;
}

static bool tap_save(TapData *tap) {
	*((uint32_t *) (tap->raw + 0x10)) = (uint32_t) (tap->end - tap->data);
	return file_save_binary(tap->file_path, tap->raw, (size_t) ((int8_t *) tap->end - tap->raw));
}

static void tap_unload(TapData *tap) {
	assert(tap);
	arrfree(tap->raw);
	free(tap->file_path);
	tap->raw = NULL;
}

static inline int64_t tap_current_interval_ps(TapData *tap) {

	static const int C64_PAL_FREQUENCY = 985248;
	int64_t interval_ps;

	if (tap->version == 1 && *tap->current == 0) {
		interval_ps = (tap->current[1] + (tap->current[2] << 8) + (tap->current[3] << 16)) / C64_PAL_FREQUENCY;
	} else {
		interval_ps = S_TO_PS((*tap->current * 8)) / C64_PAL_FREQUENCY;
	}

	return interval_ps;
}

static inline bool tap_next_sample(TapData *tap) {
	if (tap->current == tap->end) {
		return false;
	}

	tap->current += (tap->version == 1 && *tap->current == 0) ? 4 : 1;
	return true;
}

static inline void tap_write_byte(TapData *tap, uint8_t value) {
	if (tap->current == tap->end) {
		arrpush(tap->raw, (int8_t) value);

		tap->data = (uint8_t *) (tap->raw + 0x14);
		tap->end  = (uint8_t *) (tap->raw + arrlen(tap->raw));
		tap->current = tap->end;
	} else {
		*tap->current++ = value;
	}
}

static inline void tap_write_pulse(TapData *tap, int64_t length_ps) {

	static const int C64_PAL_FREQUENCY = 985248;

	int64_t length_cycles = PS_TO_S(length_ps * C64_PAL_FREQUENCY);

	if (length_cycles > 255 * 8) {
		tap_write_byte(tap, 0x00);
		tap_write_byte(tap, (uint8_t) (length_cycles & 0xff));
		tap_write_byte(tap, (uint8_t) ((length_cycles >> 8) & 0xff));
		tap_write_byte(tap, (uint8_t) ((length_cycles >> 16) & 0xff));
	} else {
		tap_write_byte(tap, (uint8_t) (length_cycles / 8) + ((length_cycles >> 2) & 0x01));
	}
}

static inline void ds_change_state(PerifDatassette *datassette, PerifDatassetteState new_state) {

	switch (new_state) {
		case STATE_IDLE:
			datassette->valid_keys = 0;
			datassette->sense_out = ACTLO_DEASSERT;
			tap_unload(&datassette->tap);
			break;
		case STATE_TAPE_LOADED:
			datassette->valid_keys = DS_KEY_RECORD | DS_KEY_PLAY | DS_KEY_REWIND | DS_KEY_FFWD | DS_KEY_EJECT;
			datassette->sense_out = ACTLO_DEASSERT;
			break;
		case STATE_PLAYING:
		case STATE_REWINDING :
		case STATE_FFWDING :
			datassette->valid_keys = DS_KEY_STOP;
			datassette->sense_out = ACTLO_ASSERT;
			break;

		case STATE_RECORDING:
			datassette->valid_keys = DS_KEY_STOP;
			datassette->sense_out = ACTLO_ASSERT;
			datassette->record_prev_tick = 0;
			datassette->record_count = 0;
			break;
	}

	datassette->state = (int) new_state;
}

static inline void ds_fast_forward(PerifDatassette *datassette) {

	int64_t interval = 0;

	for (int i = 0; i < TAP_REW_FFWD_SAMPLES; ++i) {
		interval += tap_current_interval_ps(&datassette->tap);
		if (!tap_next_sample(&datassette->tap)) {
			ds_change_state(datassette, STATE_TAPE_LOADED);
			break;
		}
	}

	datassette->offset_ps += interval;
}

static inline void ds_rewind(PerifDatassette *datassette) {

	int64_t interval = 0;

	for (int i = 0; i < TAP_REW_FFWD_SAMPLES; ++i) {
		if (datassette->tap.current - datassette->tap.data >= 4 && *(datassette->tap.current - 4) == 0) {
			datassette->tap.current -= 4;
		} else {
			datassette->tap.current -= 1;
		}

		interval += tap_current_interval_ps(&datassette->tap);

		if (datassette->tap.current == datassette->tap.data) {
			ds_change_state(datassette, STATE_TAPE_LOADED);
			break;
		}
	}

	datassette->offset_ps -= interval;
	datassette->offset_ps = MAX(datassette->offset_ps, 0);
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
	signal_add_dependency(SIGNAL_POOL, SIGNAL(data_to_ds), datassette->id);
}

void perif_datassette_process(PerifDatassette *datassette) {
	assert(datassette);

	// sense signal
	SIGNAL_SET_BOOL(sense, datassette->sense_out);

	// recording - timing is driven by the signals from the computer
	if (datassette->state == STATE_RECORDING) {
		bool motor = SIGNAL_BOOL(motor);

		// write a sample on the positive edge of data_to_ds
		if (motor && SIGNAL_BOOL(data_to_ds) && signal_changed(SIGNAL_POOL, SIGNAL(data_to_ds))) {
			if (datassette->record_prev_tick > 0) {
				int64_t length_ticks = datassette->simulator->current_tick - datassette->record_prev_tick;
				datassette->record_count += 1;
				tap_write_pulse(&datassette->tap, length_ticks * datassette->simulator->tick_duration_ps);
				datassette->offset_ps += length_ticks * datassette->simulator->tick_duration_ps;
			}
			datassette->record_prev_tick = datassette->simulator->current_tick;
		}

		// write the .tap file when the motor stops
		if (!motor && datassette->record_count > 0) {
			tap_save(&datassette->tap);
			datassette->record_count = 0;
		}

		return;
	}

	// don't do anything if motor isn't being driven
	if (!SIGNAL_BOOL(motor)) {
		// schedule sense signal check
		datassette->schedule_timestamp = datassette->simulator->current_tick + datassette->idle_interval;
		return;
	}

	// non-recording - timing is driven by the datassette
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
			if (!tap_next_sample(&datassette->tap)) {
				ds_change_state(datassette, STATE_TAPE_LOADED);
			}
		}

		SIGNAL_SET_BOOL(data_from_ds, datassette->data_out);
	} else if (datassette->state == STATE_REWINDING && datassette->tick_next_transition <= datassette->simulator->current_tick) {
		ds_rewind(datassette);
		datassette->tick_next_transition = datassette->simulator->current_tick + datassette->idle_interval;
	} else if (datassette->state == STATE_FFWDING && datassette->tick_next_transition <= datassette->simulator->current_tick) {
		ds_fast_forward(datassette);
		datassette->tick_next_transition = datassette->simulator->current_tick + datassette->idle_interval;
	} else if (datassette->state <= STATE_TAPE_LOADED && datassette->tick_next_transition <= datassette->simulator->current_tick) {
		datassette->tick_next_transition = datassette->simulator->current_tick + datassette->idle_interval;
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
			ds_change_state(datassette, STATE_RECORDING);
			break;
		case DS_KEY_PLAY :
			ds_change_state(datassette, STATE_PLAYING);
			break;
		case DS_KEY_REWIND :
			ds_change_state(datassette, STATE_REWINDING);
			break;
		case DS_KEY_FFWD :
			ds_change_state(datassette, STATE_FFWDING);
			break;
		case DS_KEY_STOP :
			ds_change_state(datassette, STATE_TAPE_LOADED);
			break;
		case DS_KEY_EJECT :
			ds_change_state(datassette, STATE_IDLE);
			break;
	}
}

void perif_datassette_load_tap(PerifDatassette *datassette, const char *filename) {
	assert(datassette);
	assert(filename);

	if (tap_load(filename, &datassette->tap)) {
		ds_change_state(datassette, STATE_TAPE_LOADED);
	}
}

void perif_datassette_new_tap(PerifDatassette *datassette, const char *filename) {
	assert(datassette);
	assert(filename);

	if (tap_new(filename, &datassette->tap)) {
		ds_change_state(datassette, STATE_TAPE_LOADED);
	}
}
