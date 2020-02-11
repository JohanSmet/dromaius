// input_keypad.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// 16 key (4 rows, 4 columns) keypad

#ifndef DROMAIUS_INPUT_KEYPAD_H
#define DROMAIUS_INPUT_KEYPAD_H

#include "types.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct InputKeypadSignals {
	Signal	rows;				// 4-bit (input)
	Signal	cols;				// 4-bit (output)

	// separate column signals
	Signal	c0;
	Signal	c1;
	Signal	c2;
	Signal	c3;
} InputKeypadSignals;

typedef struct InputKeypad {
	// interface
	SignalPool *		signal_pool;
	InputKeypadSignals	signals;

	// data
	bool				keys[4][4];
} InputKeypad;

// functions
InputKeypad *input_keypad_create(SignalPool *pool, InputKeypadSignals signals);
void input_keypad_destroy(InputKeypad *keypad);
void input_keypad_process(InputKeypad *keypad);

void input_keypad_key_pressed(InputKeypad *keypad, size_t row, size_t col);
void input_keypad_set_decay(InputKeypad *keypad, int cycles);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_INPUT_KEYPAD_H
