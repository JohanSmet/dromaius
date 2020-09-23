// input_keypad.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// configurable keypad

#include "input_keypad.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			keypad->signal_pool
#define SIGNAL_COLLECTION	keypad->signals
#define SIGNAL_CHIP_ID		keypad->id

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
} InputKeypadPrivate;

#define PRIVATE(x)	((InputKeypadPrivate *) (x))

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

InputKeypad *input_keypad_create(SignalPool *pool,
								 bool active_high,
								 size_t row_count, size_t col_count,
								 int dwell_ms,
								 int matrix_scan_frequency,
								 InputKeypadSignals signals) {
	InputKeypadPrivate *priv = calloc(1, sizeof(InputKeypadPrivate));
	InputKeypad *keypad = &priv->intf;
	CHIP_SET_FUNCTIONS(keypad, input_keypad_process, input_keypad_destroy, input_keypad_register_dependencies);

	keypad->keys = (bool *) calloc(row_count * col_count, sizeof(bool));
	priv->key_release_ticks = (int64_t *) calloc(row_count * col_count, sizeof(int64_t));
	priv->keys_down = (size_t *) calloc(row_count * col_count, sizeof(size_t));

	keypad->signal_pool = pool;
	keypad->active_high = active_high;
	keypad->row_count = row_count;
	keypad->col_count = col_count;
	keypad->key_count = row_count * col_count;
	priv->keypad_scan_interval = signal_pool_interval_to_tick_count(pool, FREQUENCY_TO_PS(matrix_scan_frequency));
	priv->next_keypad_scan_tick = priv->keypad_scan_interval;
	priv->key_dwell_cycles = signal_pool_interval_to_tick_count(pool, MS_TO_PS(dwell_ms));

	memcpy(&keypad->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(rows, (uint32_t) row_count);
	SIGNAL_DEFINE(cols, (uint32_t) col_count);

	return keypad;
}

void input_keypad_register_dependencies(InputKeypad *keypad) {
	assert(keypad);
	signal_add_dependency(keypad->signal_pool, SIGNAL(rows), keypad->id);
}

void input_keypad_destroy(InputKeypad *keypad) {
	assert(keypad);
	free(keypad->keys);
	free(PRIVATE(keypad)->key_release_ticks);
	free(PRIVATE(keypad));
}

void input_keypad_process(InputKeypad *keypad) {
	assert(keypad);

	if (PRIVATE(keypad)->keys_down_count == 0) {
		SIGNAL_SET_UINT16(cols, (uint16_t) (keypad->active_high - 1));
		return;
	}

	// update key-decay and refresh keys_down array
	int64_t current_tick = keypad->signal_pool->current_tick;

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
	SIGNAL_SET_UINT16(cols, (uint16_t) (keypad->active_high - 1));

	// >> set bits for pressed keys of selected row
	for (size_t i = 0; i < PRIVATE(keypad)->keys_down_count; ++i) {
		uint32_t k = (uint32_t) PRIVATE(keypad)->keys_down[i];
		uint32_t r = k / (uint32_t) keypad->col_count;
		uint32_t c = k - (r * (uint32_t) keypad->col_count);

		bool input = signal_read_bool(keypad->signal_pool, signal_split(SIGNAL(rows), r, 1));
		if (input != keypad->active_high) {
			continue;
		}
		signal_write_bool(keypad->signal_pool, signal_split(SIGNAL(cols), c, 1), input, keypad->id);
	}
}

void input_keypad_key_pressed(InputKeypad *keypad, size_t row, size_t col) {
	assert(keypad);
	assert(row < keypad->row_count);
	assert(col < keypad->col_count);

	const int64_t current_tick = keypad->signal_pool->current_tick;
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

	PRIVATE(keypad)->key_dwell_cycles = signal_pool_interval_to_tick_count(keypad->signal_pool, dwell_ms * 1000000000ll);
}

size_t input_keypad_keys_down_count(InputKeypad *keypad) {
	assert(keypad);
	return PRIVATE(keypad)->keys_down_count;
}

size_t* input_keypad_keys_down(InputKeypad *keypad) {
	assert(keypad);
	return PRIVATE(keypad)->keys_down;
}
