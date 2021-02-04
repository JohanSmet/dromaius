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
typedef struct InputKeypad {
	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	Signal *			signals;

	SignalGroup			sg_rows;
	SignalGroup			sg_cols;

	// data
	bool				active_high;
	size_t				row_count;
	size_t				col_count;
	size_t				key_count;
	bool				*keys;
} InputKeypad;

// functions
InputKeypad *input_keypad_create(struct Simulator *sim,
								 bool active_high,
								 size_t row_count, size_t col_count,
								 int dwell_ms,
								 int matrix_scan_frequency,
								 Signal *row_signals,
								 Signal *col_signals
);

void input_keypad_register_dependencies(InputKeypad *keypad);
void input_keypad_destroy(InputKeypad *keypad);
void input_keypad_process(InputKeypad *keypad);

void input_keypad_key_pressed(InputKeypad *keypad, size_t row, size_t col);
void input_keypad_set_dwell_time_ms(InputKeypad *keypad, int dwell_ms);

size_t input_keypad_keys_down_count(InputKeypad *keypad);
size_t* input_keypad_keys_down(InputKeypad *keypad);

static inline size_t input_keypad_row_col_to_index(InputKeypad *keypad, size_t row, size_t col) {
	return (row * keypad->col_count) + col;
}

static inline void input_keypad_index_to_row_col(InputKeypad *keypad, size_t index, uint32_t *row, uint32_t *col) {
	*row = (uint32_t) (index / keypad->col_count);
	*col = (uint32_t) (index - (*row * keypad->col_count));
}

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_INPUT_KEYPAD_H
