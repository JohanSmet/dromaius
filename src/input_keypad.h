// input_keypad.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// 16 key (4 rows, 4 columns) keypad

#ifndef DROMAIUS_INPUT_KEYPAD_H
#define DROMAIUS_INPUT_KEYPAD_H

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct InputKeypadSignals {
	Signal	rows;				// n-bit (input)
	Signal	cols;				// n-bit (output)
} InputKeypadSignals;

typedef struct InputKeypad {
	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	InputKeypadSignals	signals;

	// data
	bool				active_high;
	size_t				row_count;
	size_t				col_count;
	size_t				key_count;
	bool				*keys;
} InputKeypad;

// functions
InputKeypad *input_keypad_create(SignalPool *pool, bool active_high, size_t row_count, size_t col_count,  InputKeypadSignals signals);
void input_keypad_register_dependencies(InputKeypad *keypad);
void input_keypad_destroy(InputKeypad *keypad);
void input_keypad_process(InputKeypad *keypad);

void input_keypad_key_pressed(InputKeypad *keypad, size_t row, size_t col);
void input_keypad_set_decay(InputKeypad *keypad, int cycles);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_INPUT_KEYPAD_H
