// test/test_chip_hd44780.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_hd44780.h"

#define SIGNAL_POOL			lcd->signal_pool
#define SIGNAL_COLLECTION	lcd->signals

#define LCD_CYCLE_START				\
	for (int i = 0; i < 2; ++i) {
#define LCD_CYCLE_END				\
		half_clock_cycle(lcd);		\
	}

static void *chip_hd44780_setup(const MunitParameter params[], void *user_data) {
	ChipHd44780 *lcd = chip_hd44780_create(NULL, signal_pool_create(), (ChipHd44780Signals) {0});
	return lcd;
}

static void chip_hd44780_teardown(void *fixture) {
	chip_hd44780_destroy((ChipHd44780 *) fixture);
}

static inline void half_clock_cycle(ChipHd44780 *lcd) {
	SIGNAL_SET_BOOL(enable, !SIGNAL_BOOL(enable));
	signal_pool_cycle(lcd->signal_pool);
	chip_hd44780_process(lcd);
}

static MunitResult test_write_data(const MunitParameter params[], void *user_data_or_fixture) {
	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	munit_assert_uint8(lcd->reg_ac, ==, 0);

	LCD_CYCLE_START
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(rs, true);
		SIGNAL_SET_UINT8(bus_data, (uint8_t) 'D');
	LCD_CYCLE_END

	munit_assert_uint8(lcd->reg_ac, ==, 1);
	munit_assert_uint8(lcd->ddram[0], ==, (uint8_t) 'D');
	munit_assert_uint8(lcd->reg_data, ==, 0x20);

	LCD_CYCLE_START
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(rs, true);
		SIGNAL_SET_UINT8(bus_data, (uint8_t) 'R');
	LCD_CYCLE_END

	munit_assert_uint8(lcd->reg_ac, ==, 2);
	munit_assert_uint8(lcd->ddram[0], ==, (uint8_t) 'D');
	munit_assert_uint8(lcd->ddram[1], ==, (uint8_t) 'R');
	munit_assert_uint8(lcd->reg_data, ==, 0x20);

	return MUNIT_OK;
}

static MunitResult test_read_data(const MunitParameter params[], void *user_data_or_fixture) {
	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// setup test case
	memcpy(lcd->ddram, " DROMAIUS", 9);

	munit_assert_uint8(lcd->reg_ac, ==, 0);

	LCD_CYCLE_START
		SIGNAL_SET_BOOL(rs, true);
	LCD_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, (uint8_t) ' ');
	munit_assert_uint8(lcd->reg_ac, ==, 1);
	munit_assert_uint8(lcd->reg_data, ==, 'D');

	LCD_CYCLE_START
		SIGNAL_SET_BOOL(rs, true);
	LCD_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, (uint8_t) 'D');
	munit_assert_uint8(lcd->reg_ac, ==, 2);
	munit_assert_uint8(lcd->reg_data, ==, 'R');

	LCD_CYCLE_START
		SIGNAL_SET_BOOL(rs, true);
	LCD_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, (uint8_t) 'R');
	munit_assert_uint8(lcd->reg_ac, ==, 3);
	munit_assert_uint8(lcd->reg_data, ==, 'O');

	return MUNIT_OK;
}

MunitTest chip_hd44780_tests[] = {
	{ "/write_data", test_write_data, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_data", test_read_data, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
