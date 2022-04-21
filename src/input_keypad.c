// input_keypad.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// configurable keypad

#include "input_keypad.h"
#include "simulator.h"
#include "crt.h"

#define SIGNAL_OWNER		keypad
#define SIGNAL_PREFIX

//////////////////////////////////////////////////////////////////////////////
//
// internal types
//

typedef struct InputKeypadPrivate {
	InputKeypad		intf;

	int64_t			*key_release_ticks;
	int64_t			key_dwell_cycles;

	int64_t			next_keypad_scan_tick;
	int64_t			keypad_scan_interval;

	size_t			*keys_down;
	size_t			keys_down_count;

	int				dwell_ms;
	int				matrix_scan_frequency;
} InputKeypadPrivate;

#define PRIVATE(x)	((InputKeypadPrivate *) (x))

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

InputKeypad *input_keypad_create(Simulator *sim,
								 bool active_high,
								 size_t row_count, size_t col_count,
								 int dwell_ms,
								 int matrix_scan_frequency,
								 Signal *row_signals,
								 Signal *col_signals) {

	assert((row_signals == NULL && col_signals == NULL) || (row_signals != NULL && col_signals != NULL));

	InputKeypadPrivate *priv = dms_calloc(1, sizeof(InputKeypadPrivate));
	InputKeypad *keypad = &priv->intf;
	keypad->signals = dms_calloc(row_count + col_count, sizeof(Signal));
	uint8_t *pin_types = dms_calloc(row_count + col_count, sizeof(uint8_t));

	CHIP_SET_FUNCTIONS(keypad, input_keypad_process, input_keypad_destroy);
	CHIP_SET_VARIABLES(keypad, sim, keypad->signals, pin_types, (uint32_t) (row_count + col_count));

	keypad->keys = (bool *) dms_calloc(row_count * col_count, sizeof(bool));
	priv->key_release_ticks = (int64_t *) dms_calloc(row_count * col_count, sizeof(int64_t));
	priv->keys_down = (size_t *) dms_calloc(row_count * col_count, sizeof(size_t));

	keypad->signal_pool = sim->signal_pool;
	keypad->active_high = active_high;
	keypad->row_count = row_count;
	keypad->col_count = col_count;
	keypad->key_count = row_count * col_count;
	priv->dwell_ms = dwell_ms;
	priv->matrix_scan_frequency = matrix_scan_frequency;

	priv->keypad_scan_interval = simulator_interval_to_tick_count(keypad->simulator, FREQUENCY_TO_PS(priv->matrix_scan_frequency));
	priv->next_keypad_scan_tick = priv->keypad_scan_interval;
	priv->key_dwell_cycles = simulator_interval_to_tick_count(keypad->simulator, MS_TO_PS(priv->dwell_ms));

	if (row_signals == NULL || col_signals == NULL) {
		keypad->sg_rows = signal_group_create();
		keypad->sg_cols = signal_group_create();

		for (size_t i = 0; i < row_count; ++i) {
			SIGNAL_DEFINE_GROUP(i, rows);
		}
		for (size_t i = 0; i < col_count; ++i) {
			SIGNAL_DEFINE_GROUP(row_count + i, cols);
		}
	}
	else {
		dms_memcpy(keypad->signals, row_signals, row_count * sizeof(Signal));
		dms_memcpy(keypad->signals + row_count, col_signals, col_count * sizeof(Signal));

		keypad->sg_rows = signal_group_create_from_array(row_count, keypad->signals);
		keypad->sg_cols = signal_group_create_from_array(col_count, keypad->signals + row_count);
	}

	// setup pin types
	dms_memset(pin_types, CHIP_PIN_INPUT | CHIP_PIN_TRIGGER, row_count);
	dms_memset(pin_types + row_count, CHIP_PIN_OUTPUT, col_count);

	return keypad;
}

void input_keypad_destroy(InputKeypad *keypad) {
	assert(keypad);
	dms_free(keypad->keys);
	dms_free(keypad->signals);
	dms_free(keypad->pin_types);
	dms_free(PRIVATE(keypad)->key_release_ticks);
	dms_free(PRIVATE(keypad)->keys_down);
	dms_free(PRIVATE(keypad));
}

void input_keypad_process(InputKeypad *keypad) {
	assert(keypad);

	if (PRIVATE(keypad)->keys_down_count == 0) {
		SIGNAL_GROUP_WRITE(cols, (uint16_t) (keypad->active_high - 1));
		return;
	}

	// update key-decay and refresh keys_down array
	int64_t current_tick = keypad->simulator->current_tick;

	if (current_tick >= PRIVATE(keypad)->next_keypad_scan_tick) {
		size_t kdw = 0;

		for (size_t kdr = 0; kdr < PRIVATE(keypad)->keys_down_count; ++kdr) {
			size_t k = PRIVATE(keypad)->keys_down[kdr];

			if (PRIVATE(keypad)->key_release_ticks[k] > current_tick) {
				PRIVATE(keypad)->keys_down[kdw++] = k;
				keypad->keys[k] = true;
			} else {
				keypad->keys[k] = false;
			}
		}

		PRIVATE(keypad)->keys_down_count = kdw;

		// schedule next wakeup/scan time
		PRIVATE(keypad)->next_keypad_scan_tick = current_tick + PRIVATE(keypad)->keypad_scan_interval;
		keypad->schedule_timestamp = PRIVATE(keypad)->next_keypad_scan_tick;
	}

	// output signals

	// >> reset entire output to not asserted values
	SIGNAL_GROUP_WRITE(cols, (uint16_t) (keypad->active_high - 1));

	// >> set bits for pressed keys of selected row
	for (size_t i = 0; i < PRIVATE(keypad)->keys_down_count; ++i) {
		uint32_t k = (uint32_t) PRIVATE(keypad)->keys_down[i];
		uint32_t r = k / (uint32_t) keypad->col_count;
		uint32_t c = k - (r * (uint32_t) keypad->col_count);

		bool input = signal_read(SIGNAL_POOL, *keypad->sg_rows[r]);
		if (input != keypad->active_high) {
			continue;
		}
		signal_write(SIGNAL_POOL, *keypad->sg_cols[c], input);
	}
}

void input_keypad_key_pressed(InputKeypad *keypad, size_t row, size_t col) {
	assert(keypad);
	assert(row < keypad->row_count);
	assert(col < keypad->col_count);

	const int64_t current_tick = keypad->simulator->current_tick;
	size_t k = (row * keypad->col_count) + col;

	if (PRIVATE(keypad)->key_release_ticks[k] <= current_tick) {
		PRIVATE(keypad)->keys_down[PRIVATE(keypad)->keys_down_count++] = k;
	}

	PRIVATE(keypad)->key_release_ticks[k] = current_tick + PRIVATE(keypad)->key_dwell_cycles;
	keypad->keys[k] = true;
}

void input_keypad_set_dwell_time_ms(InputKeypad *keypad, int dwell_ms) {
	assert(keypad);
	assert(dwell_ms > 0);

	PRIVATE(keypad)->key_dwell_cycles = simulator_interval_to_tick_count(keypad->simulator, dwell_ms * 1000000000ll);
}

size_t input_keypad_keys_down_count(InputKeypad *keypad) {
	assert(keypad);
	return PRIVATE(keypad)->keys_down_count;
}

size_t* input_keypad_keys_down(InputKeypad *keypad) {
	assert(keypad);
	return PRIVATE(keypad)->keys_down;
}
