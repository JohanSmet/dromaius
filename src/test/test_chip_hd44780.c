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

#define LCD_5X8_CHECK_CELL(mem, l, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf)		\
	munit_assert_##c0(lcd_5x8_char_non_empty((mem), 0, (l), 80));										\
	munit_assert_##c1(lcd_5x8_char_non_empty((mem), 1, (l), 80));										\
	munit_assert_##c2(lcd_5x8_char_non_empty((mem), 2, (l), 80));										\
	munit_assert_##c3(lcd_5x8_char_non_empty((mem), 3, (l), 80));										\
	munit_assert_##c4(lcd_5x8_char_non_empty((mem), 4, (l), 80));										\
	munit_assert_##c5(lcd_5x8_char_non_empty((mem), 5, (l), 80));										\
	munit_assert_##c6(lcd_5x8_char_non_empty((mem), 6, (l), 80));										\
	munit_assert_##c7(lcd_5x8_char_non_empty((mem), 7, (l), 80));										\
	munit_assert_##c8(lcd_5x8_char_non_empty((mem), 8, (l), 80));										\
	munit_assert_##c9(lcd_5x8_char_non_empty((mem), 9, (l), 80));										\
	munit_assert_##ca(lcd_5x8_char_non_empty((mem), 10, (l), 80));										\
	munit_assert_##cb(lcd_5x8_char_non_empty((mem), 11, (l), 80));										\
	munit_assert_##cc(lcd_5x8_char_non_empty((mem), 12, (l), 80));										\
	munit_assert_##cd(lcd_5x8_char_non_empty((mem), 13, (l), 80));										\
	munit_assert_##ce(lcd_5x8_char_non_empty((mem), 14, (l), 80));										\
	munit_assert_##cf(lcd_5x8_char_non_empty((mem), 15, (l), 80));

#define LCD_5X8_CHECK_CURSOR(mem, l, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf)	\
	munit_assert_##c0(lcd_5x8_cursor_at_pos((mem), 0, (l), 80));										\
	munit_assert_##c1(lcd_5x8_cursor_at_pos((mem), 1, (l), 80));										\
	munit_assert_##c2(lcd_5x8_cursor_at_pos((mem), 2, (l), 80));										\
	munit_assert_##c3(lcd_5x8_cursor_at_pos((mem), 3, (l), 80));										\
	munit_assert_##c4(lcd_5x8_cursor_at_pos((mem), 4, (l), 80));										\
	munit_assert_##c5(lcd_5x8_cursor_at_pos((mem), 5, (l), 80));										\
	munit_assert_##c6(lcd_5x8_cursor_at_pos((mem), 6, (l), 80));										\
	munit_assert_##c7(lcd_5x8_cursor_at_pos((mem), 7, (l), 80));										\
	munit_assert_##c8(lcd_5x8_cursor_at_pos((mem), 8, (l), 80));										\
	munit_assert_##c9(lcd_5x8_cursor_at_pos((mem), 9, (l), 80));										\
	munit_assert_##ca(lcd_5x8_cursor_at_pos((mem), 10, (l), 80));										\
	munit_assert_##cb(lcd_5x8_cursor_at_pos((mem), 11, (l), 80));										\
	munit_assert_##cc(lcd_5x8_cursor_at_pos((mem), 12, (l), 80));										\
	munit_assert_##cd(lcd_5x8_cursor_at_pos((mem), 13, (l), 80));										\
	munit_assert_##ce(lcd_5x8_cursor_at_pos((mem), 14, (l), 80));										\
	munit_assert_##cf(lcd_5x8_cursor_at_pos((mem), 15, (l), 80));

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

static inline int lcd_5x8_char_lit_count(uint8_t *lcd_mem, int x, int y, int stride) {
	uint8_t *ptr = lcd_mem + (y * 8 * stride) + (x * 5);
	int sum = 0;

	for (int l = 0; l < 8; ++l) {
		sum += ptr[0] + ptr[1] + ptr[2] + ptr[3] + ptr[4];
		ptr += stride;
	}

	return sum;
}

static inline bool lcd_5x8_char_non_empty(uint8_t *lcd_mem, int x, int y, int stride) {
	int sum = lcd_5x8_char_lit_count(lcd_mem, x, y, stride);
	return sum > 0;
}

static inline bool lcd_5x8_cursor_at_pos(uint8_t *lcd_mem, int x, int y, int stride) {
	uint8_t *ptr = lcd_mem + (((y * 8) + 7) * stride) + (x * 5);
	int sum = ptr[0] + ptr[1] + ptr[2] + ptr[3] + ptr[4];
	return sum == 5;
}

static inline void lcd_write_char(ChipHd44780 *lcd, char c) {
	LCD_CYCLE_START
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(rs, true);
		SIGNAL_SET_UINT8(bus_data, (uint8_t) c);
	LCD_CYCLE_END
}

static inline void lcd_write_cmd(ChipHd44780 *lcd, uint8_t cmd) {
	LCD_CYCLE_START
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(rs, false);
		SIGNAL_SET_UINT8(bus_data, cmd);
	LCD_CYCLE_END
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

static MunitResult test_display_on_off_control(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// screen starts out empty
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// write character
	lcd_write_char(lcd, 'D');

	// screen still empty because it's turned off
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// turn on display
	lcd_write_cmd(lcd, 0b00001100);

	// one character shown, no cursor
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// turn off display
	lcd_write_cmd(lcd, 0b00001000);

	// screen empty
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// turn on display and cursor
	lcd_write_cmd(lcd, 0b00001110);

	// one character shown + cursor in the next position
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, true,  false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// turn off cursor
	lcd_write_cmd(lcd, 0b00001100);

	// one character displayed, no cursor
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	return MUNIT_OK;
}

static MunitResult test_return_home(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// screen starts out empty
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// write some characters
	lcd_write_char(lcd, 'D');
	lcd_write_char(lcd, 'R');
	lcd_write_char(lcd, 'O');
	lcd_write_char(lcd, 'M');

	// turn on display + cursor
	lcd_write_cmd(lcd, 0b00001110);

	munit_assert_memory_equal(4, lcd->ddram, "DROM");

	// 4 cells filed with chars, cursor in the next cell
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  true,  true,  true,  false, false, false,
		false, false, false, false, false, false, false, false);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, false, false, false, true,  false, false, false,
		false, false, false, false, false, false, false, false);

	// return home
	lcd_write_cmd(lcd, 0b00000010);

	// 4 cells filed with chars, cursor in the first cell
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  true,  true,  false, false, false, false,
		false, false, false, false, false, false, false, false);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// write a char - should overwrite the first position, cursor in the second position
	lcd_write_char(lcd, 'C');

	munit_assert_memory_equal(4, lcd->ddram, "CROM");

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  true,  true,  false, false, false, false,
		false, false, false, false, false, false, false, false);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, true,  false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	return MUNIT_OK;
}

static MunitResult test_clear_display(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// screen starts out empty
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// write some characters
	lcd_write_char(lcd, 'D');
	lcd_write_char(lcd, 'R');
	lcd_write_char(lcd, 'O');
	lcd_write_char(lcd, 'M');

	munit_assert_memory_equal(4, lcd->ddram, "DROM");
	munit_assert_uint8(lcd->reg_ac, ==, 4);

	// turn on display + cursor
	lcd_write_cmd(lcd, 0b00001110);

	// 4 cells filed with chars, cursor in the next cell
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  true,  true,  true,  false, false, false,
		false, false, false, false, false, false, false, false);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, false, false, false, true,  false, false, false,
		false, false, false, false, false, false, false, false);

	// clear display
	lcd_write_cmd(lcd, 0b00000001);

	// screen empty, cursor in first position
	munit_assert_uint8(lcd->reg_ac, ==, 0);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	return MUNIT_OK;
}

static MunitResult test_set_ddram_address_1l(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// screen starts out empty
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// turn on display
	lcd_write_cmd(lcd, 0b00001100);

	// change ddram address
	lcd_write_cmd(lcd, 0b10000000 | 2);
	munit_assert_uint8(lcd->reg_ac, ==, 2);

	// write character
	lcd_write_char(lcd, 'D');
	munit_assert_memory_equal(4, lcd->ddram, "  D                                                                              ");

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, true,  false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// change ddram address
	lcd_write_cmd(lcd, 0b10000000 | 15);
	munit_assert_uint8(lcd->reg_ac, ==, 15);

	// write character
	lcd_write_char(lcd, 'R');
	munit_assert_memory_equal(80, lcd->ddram, "  D            R                                                                ");

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, true,  false, false, false, false, false,
		false, false, false, false, false, false, false, true);

	// change ddram address
	lcd_write_cmd(lcd, 0b10000000 | 40);
	munit_assert_uint8(lcd->reg_ac, ==, 40);

	// write character
	lcd_write_char(lcd, 'O');
	munit_assert_memory_equal(80, lcd->ddram, "  D            R                        O                                       ");

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, true,  false, false, false, false, false,
		false, false, false, false, false, false, false, true);

	// change ddram address
	lcd_write_cmd(lcd, 0b10000000 | 79);
	munit_assert_uint8(lcd->reg_ac, ==, 79);

	// write character
	lcd_write_char(lcd, 'M');
	munit_assert_memory_equal(80, lcd->ddram, "  D            R                        O                                      M");

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, true,  false, false, false, false, false,
		false, false, false, false, false, false, false, true);

	return MUNIT_OK;
}

static MunitResult test_set_ddram_address_2l(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// set lcd to 2 line mode
	lcd_write_cmd(lcd, 0b00111000);
	munit_assert_uint8(lcd->display_width, ==, 16);
	munit_assert_uint8(lcd->display_height, ==, 2);

	// screen starts out empty
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// turn on display
	lcd_write_cmd(lcd, 0b00001100);

	// change ddram address
	lcd_write_cmd(lcd, 0b10000000 | 2);
	munit_assert_uint8(lcd->reg_ac, ==, 2);

	// write character
	lcd_write_char(lcd, 'D');
	munit_assert_memory_equal(4, lcd->ddram, "  D                                                                              ");

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, true,  false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// change ddram address
	lcd_write_cmd(lcd, 0b10000000 | 15);
	munit_assert_uint8(lcd->reg_ac, ==, 15);

	// write character
	lcd_write_char(lcd, 'R');
	munit_assert_memory_equal(80, lcd->ddram, "  D            R                                                                ");

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, true,  false, false, false, false, false,
		false, false, false, false, false, false, false, true);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// change ddram address
	lcd_write_cmd(lcd, 0b10000000 | 0x40 + 3);
	munit_assert_uint8(lcd->reg_ac, ==, 0x40 + 3);

	// write character
	lcd_write_char(lcd, 'O');
	munit_assert_memory_equal(80, lcd->ddram, "  D            R                           O                                    ");

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, true,  false, false, false, false, false,
		false, false, false, false, false, false, false, true);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		false, false, false, true,  false, false, false, false,
		false, false, false, false, false, false, false, false);

	// change ddram address
	lcd_write_cmd(lcd, 0b10000000 | 0x40 + 39);
	munit_assert_uint8(lcd->reg_ac, ==, 0x40 + 39);

	// write character
	lcd_write_char(lcd, 'M');
	munit_assert_memory_equal(80, lcd->ddram, "  D            R                           O                                   M");

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, true,  false, false, false, false, false,
		false, false, false, false, false, false, false, true);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		false, false, false, true,  false, false, false, false,
		false, false, false, false, false, false, false, false);

	return MUNIT_OK;
}

static MunitResult test_shift_display_1l(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// screen starts out empty
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// enable display shift
	lcd_write_cmd(lcd, 0b00000111);

	// write some characters
	lcd_write_char(lcd, 'A');
	lcd_write_char(lcd, 'B');
	lcd_write_char(lcd, ' ');
	lcd_write_char(lcd, 'C');
	lcd_write_char(lcd, 'D');
	lcd_write_char(lcd, 'E');

	lcd_write_cmd(lcd, 0b10000000 | 79);		// change address
	lcd_write_char(lcd, 'E');

	// turn on display
	lcd_write_cmd(lcd, 0b00001100);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  false, true,  true,  true,  false, false,
		false, false, false, false, false, false, false, false);

	// count enabled pixels per character
	int pixel_count[16] = {0};
	for (int i = 0; i < 16; ++i) {
		pixel_count[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 0, 80);
	}
	int last_count = pixel_count[5];

	// shift left
	lcd_write_cmd(lcd, 0b00011000);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  false, true,  true,  true,  false, false, false,
		false, false, false, false, false, false, false, false);

	int new_pixel_count[16] = {0};
	for (int i = 0; i < 16; ++i) {
		new_pixel_count[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 0, 80);
	}
	munit_assert_uint8(new_pixel_count[0], ==, pixel_count[1]);
	munit_assert_uint8(new_pixel_count[1], ==, pixel_count[2]);
	munit_assert_uint8(new_pixel_count[2], ==, pixel_count[3]);
	munit_assert_uint8(new_pixel_count[3], ==, pixel_count[4]);
	munit_assert_uint8(new_pixel_count[4], ==, pixel_count[5]);
	munit_assert_uint8(new_pixel_count[5], ==, pixel_count[6]);
	munit_assert_uint8(new_pixel_count[6], ==, pixel_count[7]);
	munit_assert_uint8(new_pixel_count[7], ==, pixel_count[8]);

	// shift right
	lcd_write_cmd(lcd, 0b00011100);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  false, true,  true,  true,  false, false,
		false, false, false, false, false, false, false, false);

	for (int i = 0; i < 16; ++i) {
		new_pixel_count[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 0, 80);
	}
	munit_assert_uint8(new_pixel_count[0], ==, pixel_count[0]);
	munit_assert_uint8(new_pixel_count[1], ==, pixel_count[1]);
	munit_assert_uint8(new_pixel_count[2], ==, pixel_count[2]);
	munit_assert_uint8(new_pixel_count[3], ==, pixel_count[3]);
	munit_assert_uint8(new_pixel_count[4], ==, pixel_count[4]);
	munit_assert_uint8(new_pixel_count[5], ==, pixel_count[5]);
	munit_assert_uint8(new_pixel_count[6], ==, pixel_count[6]);
	munit_assert_uint8(new_pixel_count[7], ==, pixel_count[7]);

	// shift right
	lcd_write_cmd(lcd, 0b00011100);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  true,  false, true,  true,  true,  false,
		false, false, false, false, false, false, false, false);

	for (int i = 0; i < 16; ++i) {
		new_pixel_count[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 0, 80);
	}
	munit_assert_uint8(new_pixel_count[0], ==, last_count);
	munit_assert_uint8(new_pixel_count[1], ==, pixel_count[0]);
	munit_assert_uint8(new_pixel_count[2], ==, pixel_count[1]);
	munit_assert_uint8(new_pixel_count[3], ==, pixel_count[2]);
	munit_assert_uint8(new_pixel_count[4], ==, pixel_count[3]);
	munit_assert_uint8(new_pixel_count[5], ==, pixel_count[4]);
	munit_assert_uint8(new_pixel_count[6], ==, pixel_count[5]);
	munit_assert_uint8(new_pixel_count[7], ==, pixel_count[6]);

	return MUNIT_OK;
}

static MunitResult test_shift_display_2l(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// set lcd to 2 line mode
	lcd_write_cmd(lcd, 0b00111000);
	munit_assert_uint8(lcd->display_width, ==, 16);
	munit_assert_uint8(lcd->display_height, ==, 2);

	// screen starts out empty
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// enable display shift
	lcd_write_cmd(lcd, 0b00000111);

	// write some characters
	for (int i = 0; i < 2; ++i) {
		lcd_write_cmd(lcd, 0b10000000 | (0x40 * i));	// change address

		lcd_write_char(lcd, 'A');
		lcd_write_char(lcd, 'B');
		lcd_write_char(lcd, ' ');
		lcd_write_char(lcd, 'C');
		lcd_write_char(lcd, 'D');
		lcd_write_char(lcd, 'E');

		lcd_write_cmd(lcd, 0b10000000 | (0x40 * i) + 39);	// change address
		lcd_write_char(lcd, 'E');
	}

	// turn on display
	lcd_write_cmd(lcd, 0b00001100);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  false, true,  true,  true,  false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		true,  true,  false, true,  true,  true,  false, false,
		false, false, false, false, false, false, false, false);

	// count enabled pixels per character
	int pixel_count_l0[16] = {0};
	int pixel_count_l1[16] = {0};
	for (int i = 0; i < 16; ++i) {
		pixel_count_l0[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 0, 80);
		pixel_count_l1[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 1, 80);
	}
	int last_count = pixel_count_l0[5];

	// shift left
	lcd_write_cmd(lcd, 0b00011000);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  false, true,  true,  true,  false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		true,  false, true,  true,  true,  false, false, false,
		false, false, false, false, false, false, false, false);

	int new_pixel_count_l0[16] = {0};
	int new_pixel_count_l1[16] = {0};
	for (int i = 0; i < 16; ++i) {
		new_pixel_count_l0[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 0, 80);
		new_pixel_count_l1[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 1, 80);
	}

	munit_assert_uint8(new_pixel_count_l0[0], ==, pixel_count_l0[1]);
	munit_assert_uint8(new_pixel_count_l0[1], ==, pixel_count_l0[2]);
	munit_assert_uint8(new_pixel_count_l0[2], ==, pixel_count_l0[3]);
	munit_assert_uint8(new_pixel_count_l0[3], ==, pixel_count_l0[4]);
	munit_assert_uint8(new_pixel_count_l0[4], ==, pixel_count_l0[5]);
	munit_assert_uint8(new_pixel_count_l0[5], ==, pixel_count_l0[6]);
	munit_assert_uint8(new_pixel_count_l0[6], ==, pixel_count_l0[7]);
	munit_assert_uint8(new_pixel_count_l0[7], ==, pixel_count_l0[8]);
	munit_assert_uint8(new_pixel_count_l1[0], ==, pixel_count_l1[1]);
	munit_assert_uint8(new_pixel_count_l1[1], ==, pixel_count_l1[2]);
	munit_assert_uint8(new_pixel_count_l1[2], ==, pixel_count_l1[3]);
	munit_assert_uint8(new_pixel_count_l1[3], ==, pixel_count_l1[4]);
	munit_assert_uint8(new_pixel_count_l1[4], ==, pixel_count_l1[5]);
	munit_assert_uint8(new_pixel_count_l1[5], ==, pixel_count_l1[6]);
	munit_assert_uint8(new_pixel_count_l1[6], ==, pixel_count_l1[7]);
	munit_assert_uint8(new_pixel_count_l1[7], ==, pixel_count_l1[8]);

	// shift right
	lcd_write_cmd(lcd, 0b00011100);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  false, true,  true,  true,  false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		true,  true,  false, true,  true,  true,  false, false,
		false, false, false, false, false, false, false, false);

	for (int i = 0; i < 16; ++i) {
		new_pixel_count_l0[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 0, 80);
		new_pixel_count_l1[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 1, 80);
	}

	munit_assert_uint8(new_pixel_count_l0[0], ==, pixel_count_l0[0]);
	munit_assert_uint8(new_pixel_count_l0[1], ==, pixel_count_l0[1]);
	munit_assert_uint8(new_pixel_count_l0[2], ==, pixel_count_l0[2]);
	munit_assert_uint8(new_pixel_count_l0[3], ==, pixel_count_l0[3]);
	munit_assert_uint8(new_pixel_count_l0[4], ==, pixel_count_l0[4]);
	munit_assert_uint8(new_pixel_count_l0[5], ==, pixel_count_l0[5]);
	munit_assert_uint8(new_pixel_count_l0[6], ==, pixel_count_l0[6]);
	munit_assert_uint8(new_pixel_count_l0[7], ==, pixel_count_l0[7]);
	munit_assert_uint8(new_pixel_count_l1[0], ==, pixel_count_l1[0]);
	munit_assert_uint8(new_pixel_count_l1[1], ==, pixel_count_l1[1]);
	munit_assert_uint8(new_pixel_count_l1[2], ==, pixel_count_l1[2]);
	munit_assert_uint8(new_pixel_count_l1[3], ==, pixel_count_l1[3]);
	munit_assert_uint8(new_pixel_count_l1[4], ==, pixel_count_l1[4]);
	munit_assert_uint8(new_pixel_count_l1[5], ==, pixel_count_l1[5]);
	munit_assert_uint8(new_pixel_count_l1[6], ==, pixel_count_l1[6]);
	munit_assert_uint8(new_pixel_count_l1[7], ==, pixel_count_l1[7]);

	// shift right
	lcd_write_cmd(lcd, 0b00011100);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true,  true,  false, true,  true,  true,  false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		true,  true,  true,  false, true,  true,  true,  false,
		false, false, false, false, false, false, false, false);

	for (int i = 0; i < 16; ++i) {
		new_pixel_count_l0[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 0, 80);
		new_pixel_count_l1[i] = lcd_5x8_char_lit_count(lcd->display_data, i, 1, 80);
	}

	munit_assert_uint8(new_pixel_count_l0[0], ==, last_count);
	munit_assert_uint8(new_pixel_count_l0[1], ==, pixel_count_l0[0]);
	munit_assert_uint8(new_pixel_count_l0[2], ==, pixel_count_l0[1]);
	munit_assert_uint8(new_pixel_count_l0[3], ==, pixel_count_l0[2]);
	munit_assert_uint8(new_pixel_count_l0[4], ==, pixel_count_l0[3]);
	munit_assert_uint8(new_pixel_count_l0[5], ==, pixel_count_l0[4]);
	munit_assert_uint8(new_pixel_count_l0[6], ==, pixel_count_l0[5]);
	munit_assert_uint8(new_pixel_count_l0[7], ==, pixel_count_l0[6]);
	munit_assert_uint8(new_pixel_count_l1[0], ==, last_count);
	munit_assert_uint8(new_pixel_count_l1[1], ==, pixel_count_l1[0]);
	munit_assert_uint8(new_pixel_count_l1[2], ==, pixel_count_l1[1]);
	munit_assert_uint8(new_pixel_count_l1[3], ==, pixel_count_l1[2]);
	munit_assert_uint8(new_pixel_count_l1[4], ==, pixel_count_l1[3]);
	munit_assert_uint8(new_pixel_count_l1[5], ==, pixel_count_l1[4]);
	munit_assert_uint8(new_pixel_count_l1[6], ==, pixel_count_l1[5]);
	munit_assert_uint8(new_pixel_count_l1[7], ==, pixel_count_l1[6]);


	return MUNIT_OK;
}

static MunitResult test_move_cursor_1l(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// screen starts out empty
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	munit_assert_uint8(lcd->reg_ac, ==, 0);

	// enable display shift
	lcd_write_cmd(lcd, 0b00000111);

	// turn on display + cursor
	lcd_write_cmd(lcd, 0b00001110);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// move cursor left
	lcd_write_cmd(lcd, 0b00010000);
	munit_assert_uint8(lcd->reg_ac, ==, 79);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// move cursor right
	lcd_write_cmd(lcd, 0b00010100);
	munit_assert_uint8(lcd->reg_ac, ==, 0);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// move cursor right
	lcd_write_cmd(lcd, 0b00010100);
	munit_assert_uint8(lcd->reg_ac, ==, 1);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, true,  false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	return MUNIT_OK;
}

static MunitResult test_move_cursor_2l(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// set lcd to 2 line mode
	lcd_write_cmd(lcd, 0b00111000);
	munit_assert_uint8(lcd->display_width, ==, 16);
	munit_assert_uint8(lcd->display_height, ==, 2);

	// screen starts out empty
	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CELL(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	munit_assert_uint8(lcd->reg_ac, ==, 0);

	// turn on display + cursor
	lcd_write_cmd(lcd, 0b00001110);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CURSOR(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// move cursor left
	lcd_write_cmd(lcd, 0b00010000);
	munit_assert_uint8(lcd->reg_ac, ==, 103);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CURSOR(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// move cursor right
	lcd_write_cmd(lcd, 0b00010100);
	munit_assert_uint8(lcd->reg_ac, ==, 0);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CURSOR(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// move cursor right
	lcd_write_cmd(lcd, 0b00010100);
	munit_assert_uint8(lcd->reg_ac, ==, 1);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, true,  false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CURSOR(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// change ddram address
	lcd_write_cmd(lcd, 0b10000000 | 39);
	munit_assert_uint8(lcd->reg_ac, ==, 39);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CURSOR(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// move cursor right
	lcd_write_cmd(lcd, 0b00010100);
	munit_assert_uint8(lcd->reg_ac, ==, 64);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CURSOR(lcd->display_data, 1,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	// move cursor left
	lcd_write_cmd(lcd, 0b00010000);
	munit_assert_uint8(lcd->reg_ac, ==, 39);

	LCD_5X8_CHECK_CURSOR(lcd->display_data, 0,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);
	LCD_5X8_CHECK_CURSOR(lcd->display_data, 1,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	return MUNIT_OK;
}

static MunitResult test_cgram_write(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;

	// set cgram address 0, should direct next data write to cgram
	lcd_write_cmd(lcd, 0b01000000);
	munit_assert_uint8(lcd->reg_ac, ==, 0);

	// write data
	LCD_CYCLE_START
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(rs, true);
		SIGNAL_SET_UINT8(bus_data, 0x5f);
	LCD_CYCLE_END

	munit_assert_uint8(lcd->reg_ac, ==, 1);
	munit_assert_uint8(lcd->reg_data, ==, 0x00);
	munit_assert_uint8(lcd->ddram[0], ==, ' ');
	munit_assert_uint8(lcd->cgram[0], ==, 0x5f);

	return MUNIT_OK;
}

static MunitResult test_cgram_char(const MunitParameter params[], void *user_data_or_fixture) {

	ChipHd44780 *lcd = (ChipHd44780 *) user_data_or_fixture;
	static const uint8_t test_data[] = {0x1f, 0x15, 0x0a, 0x1f, 0x0a, 0x15, 0x1f, 0x00,
										0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00};

	// init cgram
	memcpy(lcd->cgram, test_data, 16);

	// turn on display
	lcd_write_cmd(lcd, 0b00001100);

	// write character 1
	lcd_write_char(lcd, 1);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	munit_assert_uint8(lcd->display_data[0], ==, 0);
	munit_assert_uint8(lcd->display_data[1], ==, 0);
	munit_assert_uint8(lcd->display_data[2], ==, 1);
	munit_assert_uint8(lcd->display_data[3], ==, 0);
	munit_assert_uint8(lcd->display_data[4], ==, 0);

	munit_assert_uint8(lcd->display_data[80+0], ==, 0);
	munit_assert_uint8(lcd->display_data[80+1], ==, 0);
	munit_assert_uint8(lcd->display_data[80+2], ==, 1);
	munit_assert_uint8(lcd->display_data[80+3], ==, 0);
	munit_assert_uint8(lcd->display_data[80+4], ==, 0);

	munit_assert_uint8(lcd->display_data[2*80+0], ==, 0);
	munit_assert_uint8(lcd->display_data[2*80+1], ==, 0);
	munit_assert_uint8(lcd->display_data[2*80+2], ==, 1);
	munit_assert_uint8(lcd->display_data[2*80+3], ==, 0);
	munit_assert_uint8(lcd->display_data[2*80+4], ==, 0);

	munit_assert_uint8(lcd->display_data[3*80+0], ==, 0);
	munit_assert_uint8(lcd->display_data[3*80+1], ==, 0);
	munit_assert_uint8(lcd->display_data[3*80+2], ==, 1);
	munit_assert_uint8(lcd->display_data[3*80+3], ==, 0);
	munit_assert_uint8(lcd->display_data[3*80+4], ==, 0);

	munit_assert_uint8(lcd->display_data[4*80+0], ==, 0);
	munit_assert_uint8(lcd->display_data[4*80+1], ==, 0);
	munit_assert_uint8(lcd->display_data[4*80+2], ==, 1);
	munit_assert_uint8(lcd->display_data[4*80+3], ==, 0);
	munit_assert_uint8(lcd->display_data[4*80+4], ==, 0);

	munit_assert_uint8(lcd->display_data[5*80+0], ==, 0);
	munit_assert_uint8(lcd->display_data[5*80+1], ==, 0);
	munit_assert_uint8(lcd->display_data[5*80+2], ==, 1);
	munit_assert_uint8(lcd->display_data[5*80+3], ==, 0);
	munit_assert_uint8(lcd->display_data[5*80+4], ==, 0);

	munit_assert_uint8(lcd->display_data[6*80+0], ==, 0);
	munit_assert_uint8(lcd->display_data[6*80+1], ==, 0);
	munit_assert_uint8(lcd->display_data[6*80+2], ==, 1);
	munit_assert_uint8(lcd->display_data[6*80+3], ==, 0);
	munit_assert_uint8(lcd->display_data[6*80+4], ==, 0);

	munit_assert_uint8(lcd->display_data[7*80+0], ==, 0);
	munit_assert_uint8(lcd->display_data[7*80+1], ==, 0);
	munit_assert_uint8(lcd->display_data[7*80+2], ==, 0);
	munit_assert_uint8(lcd->display_data[7*80+3], ==, 0);
	munit_assert_uint8(lcd->display_data[7*80+4], ==, 0);

	// write character 0
	lcd_write_char(lcd, 0);

	LCD_5X8_CHECK_CELL(lcd->display_data, 0,
		true,  true, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false);

	munit_assert_uint8(lcd->display_data[5], ==, 1);
	munit_assert_uint8(lcd->display_data[6], ==, 1);
	munit_assert_uint8(lcd->display_data[7], ==, 1);
	munit_assert_uint8(lcd->display_data[8], ==, 1);
	munit_assert_uint8(lcd->display_data[9], ==, 1);

	munit_assert_uint8(lcd->display_data[80+5], ==, 1);
	munit_assert_uint8(lcd->display_data[80+6], ==, 0);
	munit_assert_uint8(lcd->display_data[80+7], ==, 1);
	munit_assert_uint8(lcd->display_data[80+8], ==, 0);
	munit_assert_uint8(lcd->display_data[80+9], ==, 1);

	munit_assert_uint8(lcd->display_data[2*80+5], ==, 0);
	munit_assert_uint8(lcd->display_data[2*80+6], ==, 1);
	munit_assert_uint8(lcd->display_data[2*80+7], ==, 0);
	munit_assert_uint8(lcd->display_data[2*80+8], ==, 1);
	munit_assert_uint8(lcd->display_data[2*80+9], ==, 0);

	munit_assert_uint8(lcd->display_data[3*80+5], ==, 1);
	munit_assert_uint8(lcd->display_data[3*80+6], ==, 1);
	munit_assert_uint8(lcd->display_data[3*80+7], ==, 1);
	munit_assert_uint8(lcd->display_data[3*80+8], ==, 1);
	munit_assert_uint8(lcd->display_data[3*80+9], ==, 1);

	munit_assert_uint8(lcd->display_data[4*80+5], ==, 0);
	munit_assert_uint8(lcd->display_data[4*80+6], ==, 1);
	munit_assert_uint8(lcd->display_data[4*80+7], ==, 0);
	munit_assert_uint8(lcd->display_data[4*80+8], ==, 1);
	munit_assert_uint8(lcd->display_data[4*80+9], ==, 0);

	munit_assert_uint8(lcd->display_data[5*80+5], ==, 1);
	munit_assert_uint8(lcd->display_data[5*80+6], ==, 0);
	munit_assert_uint8(lcd->display_data[5*80+7], ==, 1);
	munit_assert_uint8(lcd->display_data[5*80+8], ==, 0);
	munit_assert_uint8(lcd->display_data[5*80+9], ==, 1);

	munit_assert_uint8(lcd->display_data[6*80+5], ==, 1);
	munit_assert_uint8(lcd->display_data[6*80+6], ==, 1);
	munit_assert_uint8(lcd->display_data[6*80+7], ==, 1);
	munit_assert_uint8(lcd->display_data[6*80+8], ==, 1);
	munit_assert_uint8(lcd->display_data[6*80+9], ==, 1);

	munit_assert_uint8(lcd->display_data[7*80+5], ==, 0);
	munit_assert_uint8(lcd->display_data[7*80+6], ==, 0);
	munit_assert_uint8(lcd->display_data[7*80+7], ==, 0);
	munit_assert_uint8(lcd->display_data[7*80+8], ==, 0);
	munit_assert_uint8(lcd->display_data[7*80+9], ==, 0);

	return MUNIT_OK;
}

MunitTest chip_hd44780_tests[] = {
	{ "/write_data", test_write_data, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_data", test_read_data, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/display_on_off_control", test_display_on_off_control, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/return_home", test_return_home, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/clear_display", test_clear_display, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/set_ddram_address_1l", test_set_ddram_address_1l, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/set_ddram_address_2l", test_set_ddram_address_2l, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/shift_display_1l", test_shift_display_1l, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/shift_display_2l", test_shift_display_2l, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/move_cursor_1l", test_move_cursor_1l, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/move_cursor_2l", test_move_cursor_2l, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/cgram_write", test_cgram_write, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/cgram_char", test_cgram_char, chip_hd44780_setup, chip_hd44780_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
