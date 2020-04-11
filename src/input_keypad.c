// input_keypad.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// configurable keypad

#include "input_keypad.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			keypad->signal_pool
#define SIGNAL_COLLECTION	keypad->signals

//////////////////////////////////////////////////////////////////////////////
//
// internal types
//

typedef struct InputKeypadPrivate {
	InputKeypad		intf;

	int				*key_decay;
	int				decay_start;
} InputKeypadPrivate;

#define PRIVATE(x)	((InputKeypadPrivate *) (x))


//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

InputKeypad *input_keypad_create(SignalPool *pool, bool active_high, size_t row_count, size_t col_count, InputKeypadSignals signals) {
	InputKeypadPrivate *priv = calloc(1, sizeof(InputKeypadPrivate));
	InputKeypad *keypad = &priv->intf;

	keypad->keys = (bool *) calloc(row_count * col_count, sizeof(bool));
	priv->key_decay = (int *) calloc(row_count * col_count, sizeof(int));

	keypad->signal_pool = pool;
	keypad->active_high = active_high;
	keypad->row_count = row_count;
	keypad->col_count = col_count;
	keypad->key_count = row_count * col_count;
	priv->decay_start = 1;

	memcpy(&keypad->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(rows, (uint32_t) row_count);
	SIGNAL_DEFINE(cols, (uint32_t) col_count);

	return keypad;
}

void input_keypad_destroy(InputKeypad *keypad) {
	assert(keypad);
	free(keypad->keys);
	free(PRIVATE(keypad)->key_decay);
	free(PRIVATE(keypad));
}

void input_keypad_process(InputKeypad *keypad) {
	assert(keypad);

	// synthesize key-down bool and decay key pressed
	for (size_t k = 0; k < keypad->key_count; ++k) {
		keypad->keys[k] = PRIVATE(keypad)->key_decay[k] > 0;
		PRIVATE(keypad)->key_decay[k] -= keypad->keys[k];
	}

	// output signals
	for (uint32_t r = 0; r < keypad->row_count; ++r) {
		bool input = signal_read_bool(keypad->signal_pool, signal_split(SIGNAL(rows), r, 1));
		if (input != keypad->active_high) {
			continue;
		}
		bool *keys = keypad->keys + (r * keypad->col_count);

		for (uint32_t c = 0; c < keypad->col_count; ++c) {
			if (keys[c]) {
				signal_write_bool(keypad->signal_pool, signal_split(SIGNAL(cols), c, 1), input);
			}
		}
	}
}

void input_keypad_key_pressed(InputKeypad *keypad, size_t row, size_t col) {
	assert(keypad);
	assert(row < keypad->row_count);
	assert(col < keypad->col_count);

	PRIVATE(keypad)->key_decay[row*keypad->col_count+col] = PRIVATE(keypad)->decay_start;
}

void input_keypad_set_decay(InputKeypad *keypad, int cycles) {
	assert(keypad);
	PRIVATE(keypad)->decay_start = cycles;
}

