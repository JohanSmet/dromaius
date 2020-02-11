// test/test_input_keypad.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "input_keypad.h"

#define SIGNAL_POOL			keypad->signal_pool
#define SIGNAL_COLLECTION	keypad->signals

static void *input_keypad_setup(const MunitParameter params[], void *user_data) {
	SignalPool *pool = signal_pool_create();
	return input_keypad_create(pool, (InputKeypadSignals){0});
}

static void input_keypad_teardown(void *fixture) {
	InputKeypad *keypad = (InputKeypad *) fixture;
	signal_pool_destroy(keypad->signal_pool);
	input_keypad_destroy(keypad);
}

static inline void run_cycle(InputKeypad *keypad) {
	signal_pool_cycle(keypad->signal_pool);
	input_keypad_process(keypad);
}

#define CHECK_ROW(row, expected)								\
	SIGNAL_SET_UINT8(rows, (row));								\
	run_cycle(keypad);											\
	munit_assert_uint8(SIGNAL_NEXT_UINT8(cols), ==, (expected));

MunitResult test_keypad(const MunitParameter params[], void *user_data_or_fixture) {

	InputKeypad *keypad = (InputKeypad *) user_data_or_fixture;

	// no keys pressed
	CHECK_ROW(0b0001, 0b0000);
	CHECK_ROW(0b0010, 0b0000);
	CHECK_ROW(0b0100, 0b0000);
	CHECK_ROW(0b1000, 0b0000);

	// one key pressed
	input_keypad_set_decay(keypad, 4);

	for (int r = 0; r < 4; ++r) {
		for (int c = 0; c < 4; ++c) {
			input_keypad_key_pressed(keypad, r, c);

			CHECK_ROW(0b0001, (r == 0) ? 1 << c : 0);
			CHECK_ROW(0b0010, (r == 1) ? 1 << c : 0);
			CHECK_ROW(0b0100, (r == 2) ? 1 << c : 0);
			CHECK_ROW(0b1000, (r == 3) ? 1 << c : 0);
		}
	}

	return MUNIT_OK;
}

MunitTest input_keypad_tests[] = {
	{ "/keypad", test_keypad, input_keypad_setup, input_keypad_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
