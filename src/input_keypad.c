// input_keypad.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// 16 key (4 rows, 4 columns) keypad

#include "input_keypad.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			keypad->signal_pool
#define SIGNAL_COLLECTION	keypad->signals

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

InputKeypad *input_keypad_create(SignalPool *pool, InputKeypadSignals signals) {
	InputKeypad *keypad = calloc(1, sizeof(InputKeypad));
	keypad->signal_pool = pool;

	memcpy(&keypad->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(rows, 4);
	SIGNAL_DEFINE(cols, 4);

	SIGNAL(c0) = signal_split(SIGNAL(cols), 0, 1);
	SIGNAL(c1) = signal_split(SIGNAL(cols), 1, 1);
	SIGNAL(c2) = signal_split(SIGNAL(cols), 2, 1);
	SIGNAL(c3) = signal_split(SIGNAL(cols), 3, 1);

	return keypad;
}

void input_keypad_destroy(InputKeypad *keypad) {
	assert(keypad);
	free(keypad);
}

void input_keypad_process(InputKeypad *keypad) {
	assert(keypad);
	
	uint8_t rows = SIGNAL_UINT8(rows);
	bool output[4] = {false, false, false, false};	

	for (int i = 0; i < 4; ++i) {
		if (IS_BIT_SET(rows, i)) {
			output[0] |= keypad->keys[i][0];
			output[1] |= keypad->keys[i][1];
			output[2] |= keypad->keys[i][2];
			output[3] |= keypad->keys[i][3];
		}
	}

	SIGNAL_SET_BOOL(c0, output[0]);
	SIGNAL_SET_BOOL(c1, output[1]);
	SIGNAL_SET_BOOL(c2, output[2]);
	SIGNAL_SET_BOOL(c3, output[3]);
}


