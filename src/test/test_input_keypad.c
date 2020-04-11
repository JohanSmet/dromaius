// test/test_input_keypad.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "input_keypad.h"

#define SIGNAL_POOL			keypad->signal_pool
#define SIGNAL_COLLECTION	keypad->signals

static void input_keypad_teardown(InputKeypad *keypad) {
	signal_pool_destroy(keypad->signal_pool);
	input_keypad_destroy(keypad);
}

static inline void run_cycle(InputKeypad *keypad) {
	signal_pool_cycle(keypad->signal_pool);
	input_keypad_process(keypad);
}

#define CHECK_ROW(row, expected)								\
	SIGNAL_SET_UINT16(rows, (row));								\
	run_cycle(keypad);											\
	munit_assert_uint8(SIGNAL_NEXT_UINT8(cols), ==, (expected));

MunitResult test_keypad_4x4(const MunitParameter params[], void *user_data_or_fixture) {

	InputKeypad *keypad = input_keypad_create(signal_pool_create(), true, 4, 4, (InputKeypadSignals) {0});

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

	input_keypad_teardown(keypad);

	return MUNIT_OK;
}

MunitResult test_keypad_10x8(const MunitParameter params[], void *user_data_or_fixture) {

	InputKeypad *keypad = input_keypad_create(signal_pool_create(), false, 10, 8, (InputKeypadSignals) {0});
	signal_default_uint8(keypad->signal_pool, SIGNAL(cols), 0xff);

	// no keys pressed
	CHECK_ROW(0b1111111110, 0xff);
	CHECK_ROW(0b1111111101, 0xff);
	CHECK_ROW(0b1111111011, 0xff);
	CHECK_ROW(0b1111110111, 0xff);
	CHECK_ROW(0b1111101111, 0xff);
	CHECK_ROW(0b1111011111, 0xff);
	CHECK_ROW(0b1110111111, 0xff);
	CHECK_ROW(0b1101111111, 0xff);
	CHECK_ROW(0b1011111111, 0xff);
	CHECK_ROW(0b0111111111, 0xff);

	// one key pressed
	input_keypad_set_decay(keypad, 10);

	for (int r = 0; r < 10; ++r) {
		for (int c = 0; c < 8; ++c) {
			input_keypad_key_pressed(keypad, r, c);

			CHECK_ROW(0b1111111110, (r == 0) ? ~(1 << c) : 0xff);
			CHECK_ROW(0b1111111101, (r == 1) ? ~(1 << c) : 0xff);
			CHECK_ROW(0b1111111011, (r == 2) ? ~(1 << c) : 0xff);
			CHECK_ROW(0b1111110111, (r == 3) ? ~(1 << c) : 0xff);
			CHECK_ROW(0b1111101111, (r == 4) ? ~(1 << c) : 0xff);
			CHECK_ROW(0b1111011111, (r == 5) ? ~(1 << c) : 0xff);
			CHECK_ROW(0b1110111111, (r == 6) ? ~(1 << c) : 0xff);
			CHECK_ROW(0b1101111111, (r == 7) ? ~(1 << c) : 0xff);
			CHECK_ROW(0b1011111111, (r == 8) ? ~(1 << c) : 0xff);
			CHECK_ROW(0b0111111111, (r == 9) ? ~(1 << c) : 0xff);
		}
	}

	input_keypad_teardown(keypad);

	return MUNIT_OK;
};

MunitTest input_keypad_tests[] = {
	{ "/keypad_4x4", test_keypad_4x4, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/keypad_10x8", test_keypad_10x8, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
