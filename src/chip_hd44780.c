// chip_hd44780.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Partial emulation of the HD44780 Dot Matrix LCD Controller/Driver

#include "chip_hd44780.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stb/stb_ds.h>

#define SIGNAL_POOL			lcd->signal_pool
#define SIGNAL_COLLECTION	lcd->signals

//////////////////////////////////////////////////////////////////////////////
//
// internal types
//

typedef enum RamMode {
	RM_DDRAM = 0,
	RM_CGRAM = 1
} RamMode;

typedef enum DataLen {
	DL_4BIT = 0,
	DL_8BIT = 1
} DataLen;

typedef struct ChipHd44780_private {
	ChipHd44780		intf;

	bool			prev_enable;
	uint8_t			address_delta;
	RamMode			ram_mode;
	DataLen			data_len;
	bool			refresh_screen;
} ChipHd44780_private;

const static uint8_t rom_a00[256][16] = {
	#include "chip_hd44780_a00.inc"
};

//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#define PRIVATE(lcd)	((ChipHd44780_private *) lcd)
#define RW_READ			true
#define RW_WRITE		false

static inline uint8_t read_register(ChipHd44780 *lcd) {

	if (SIGNAL_BOOL(rs) == false) {
		// read busy-flag and address (note: busy flag not implemented yet)
		return lcd->reg_ac & 0x7f;
	} else {
		// read data from CGRAM or DDRAM via the data register
		return lcd->reg_data;
	}

	return 0;
}

static inline uint8_t ddram_physical_addr(ChipHd44780 *lcd, uint8_t addr) {
	if (lcd->display_height == 1) {
		return addr;
	} else {
		if (addr >= 64) {
			// there's a 'virtual' gap between the first and second line
			return addr - 24;
		} else {
			return addr;
		}
	}
}

static inline void ddram_fix_address(ChipHd44780 *lcd) {
	if (lcd->display_height == 1) {
		lcd->reg_ac %= 80;
	} else {
		// first line from 0-40 (0x00-0x27) / second line from 64-103 (0x40-0x67)
		lcd->reg_ac %= 104;
		if (lcd->reg_ac > 40 && lcd->reg_ac < 64) {
			lcd->reg_ac = (PRIVATE(lcd)->address_delta > 0) ? 64 : 39;
		}
	}
}

static inline void cgram_fix_address(ChipHd44780 *lcd) {
	lcd->reg_ac &= 0x3f;
}

static inline void ddram_set_address(ChipHd44780 *lcd, uint8_t address) {
	lcd->reg_ac = address;
	ddram_fix_address(lcd);
	lcd->reg_data = lcd->ddram[ddram_physical_addr(lcd, lcd->reg_ac)];
	PRIVATE(lcd)->ram_mode = RM_DDRAM;
}

static inline void cgram_set_address(ChipHd44780 *lcd, uint8_t address) {
	lcd->reg_ac = address;
	cgram_fix_address(lcd);
	lcd->reg_data = lcd->cgram[address];
	PRIVATE(lcd)->ram_mode = RM_CGRAM;
}

static inline void increment_decrement_addres(ChipHd44780 *lcd) {
	lcd->reg_ac += PRIVATE(lcd)->address_delta;

	switch (PRIVATE(lcd)->ram_mode) {
		case RM_DDRAM:
			ddram_fix_address(lcd);
			lcd->reg_data = lcd->ddram[ddram_physical_addr(lcd, lcd->reg_ac)];
			break;
		case RM_CGRAM:
			cgram_fix_address(lcd);
			lcd->reg_data = lcd->cgram[lcd->reg_ac];
			break;
	}

}

static inline void execute_clear_display(ChipHd44780 *lcd) {
	// - write 0x20 into all DDRAM address
	// - set DDRAM address 0 into the address counter
	// - (TODO) return display to original status if it was shifted
	// - set I/D of entry mode to 1 (increment) / S doesn't change
	memset(lcd->ddram, 0x20, DDRAM_SIZE);
	memset(lcd->display_data, 0, arrlenu(lcd->display_data));
	ddram_set_address(lcd, 0);
	PRIVATE(lcd)->address_delta = 1;
	PRIVATE(lcd)->refresh_screen = true;
}

static inline void execute_return_home(ChipHd44780 *lcd) {
	// - set DDRAM address 0 into the address counter
	// - (TODO) return display to original status if it was shifted
	ddram_set_address(lcd, 0);
}

static inline void execute_entry_mode_set(ChipHd44780 *lcd, bool inc_or_dec, bool shift) {
	PRIVATE(lcd)->address_delta = (inc_or_dec) ? 1 : -1;
	// (TODO) handle display shift
}

static inline void execute_display_on_off_control(ChipHd44780 *lcd, bool display, bool cursor, bool cursor_blink) {
	lcd->display_enabled = display;
	PRIVATE(lcd)->refresh_screen = true;
	// (TODO) handle cursor and cursor_blink
}

static inline void execute_cursor_or_display_shift(ChipHd44780 *lcd, bool display_or_cursor, bool right_or_left) {
}

static inline void execute_function_set(ChipHd44780 *lcd, bool dl, bool n, bool f) {
	// dl = 1: 8-bits, dl = 0: 4-bits
	//  n = 1: 2 lines, n = 0: 1 line
	//  f = 1: 5 x 10 dots, f = 0: 5 x 8 dots
	PRIVATE(lcd)->data_len = (dl) ? DL_8BIT : DL_4BIT;

	lcd->display_width = 16;
	lcd->display_height = (n) ? 2 : 1;
	lcd->char_width = 5;
	lcd->char_height = (f) ? 10 : 8;
	arrsetlen(lcd->display_data, lcd->display_width * lcd->display_height * lcd->char_width * lcd->char_height);
	PRIVATE(lcd)->refresh_screen = true;

	assert(PRIVATE(lcd)->data_len == DL_8BIT && "Only 8-bit interface mode supported");
}

static inline void execute_set_cgram_address(ChipHd44780 *lcd, uint8_t addr) {
	cgram_set_address(lcd, addr);
}

static inline void execute_set_ddram_address(ChipHd44780 *lcd, uint8_t addr) {
	ddram_set_address(lcd, addr);
}

static inline void decode_instruction(ChipHd44780 *lcd) {
	if (IS_BIT_SET(lcd->reg_ir, 7)) {
		execute_set_ddram_address(lcd, lcd->reg_ir & 0x7f);
	} else if (IS_BIT_SET(lcd->reg_ir, 6)) {
		execute_set_cgram_address(lcd, lcd->reg_ir & 0x3f);
	} else if (IS_BIT_SET(lcd->reg_ir, 5)) {
		execute_function_set(lcd,	IS_BIT_SET(lcd->reg_ir, 4),
									IS_BIT_SET(lcd->reg_ir, 3),
									IS_BIT_SET(lcd->reg_ir, 2));
	} else if (IS_BIT_SET(lcd->reg_ir, 4)) {
		execute_cursor_or_display_shift(lcd,	IS_BIT_SET(lcd->reg_ir, 3),
												IS_BIT_SET(lcd->reg_ir, 2));
	} else if (IS_BIT_SET(lcd->reg_ir, 3)) {
		execute_display_on_off_control(lcd, IS_BIT_SET(lcd->reg_ir, 2),
											IS_BIT_SET(lcd->reg_ir, 1),
											IS_BIT_SET(lcd->reg_ir, 0));
	} else if (IS_BIT_SET(lcd->reg_ir, 2)) {
		execute_entry_mode_set(lcd, IS_BIT_SET(lcd->reg_ir, 1),
									IS_BIT_SET(lcd->reg_ir, 0));
	} else if (IS_BIT_SET(lcd->reg_ir, 1)) {
		execute_return_home(lcd);
	} else if (IS_BIT_SET(lcd->reg_ir, 0)) {
		execute_clear_display(lcd);
	}
}

static inline void store_data(ChipHd44780 *lcd) {
	switch (PRIVATE(lcd)->ram_mode) {
		case RM_CGRAM:
			lcd->cgram[lcd->reg_ac] = lcd->reg_data;
			break;
		case RM_DDRAM:
			lcd->ddram[ddram_physical_addr(lcd, lcd->reg_ac)] = lcd->reg_data;
			PRIVATE(lcd)->refresh_screen = true;
			break;
	}

	increment_decrement_addres(lcd);
}

static inline void draw_character(ChipHd44780 *lcd, uint8_t c, uint8_t x, uint8_t y) {
	const uint8_t *src = rom_a00[c];
	uint8_t *dst = lcd->display_data + (y * lcd->char_height * lcd->char_width * lcd->display_width) + (x * lcd->char_width);

	for (int l = 0; l < lcd->char_height; ++l) {
		dst[0] = (*src & 0x10) != 0;
		dst[1] = (*src & 0x08) != 0;
		dst[2] = (*src & 0x04) != 0;
		dst[3] = (*src & 0x02) != 0;
		dst[4] = (*src & 0x01) != 0;

		dst += lcd->char_width * lcd->display_width;
		src++;
	}
}

static inline void refresh_screen(ChipHd44780 *lcd) {
	if (!lcd->display_enabled) {
		memset(lcd->display_data, 0, arrlenu(lcd->display_data));
		return;
	}

	for (int l = 0; l < lcd->display_height; ++l) {
		for (int i = 0; i < 16; ++i) {
			uint8_t c = lcd->ddram[ddram_physical_addr(lcd, (0x40 * l) + i)];
			draw_character(lcd, c, i, l);
		}
	}
}

static inline void process_positive_enable_edge(ChipHd44780 *lcd) {
	if (SIGNAL_BOOL(rw) == RW_READ) {
		SIGNAL_SET_UINT8(bus_data, read_register(lcd));
	}
}

static inline void process_negative_enable_edge(ChipHd44780 *lcd) {
	// read mode
	if (SIGNAL_BOOL(rw) == RW_READ) {
		SIGNAL_SET_UINT8(bus_data, read_register(lcd));
		increment_decrement_addres(lcd);
		return;
	}

	// write mode
	if (SIGNAL_BOOL(rs) == false) {
		// instruction
		lcd->reg_ir = SIGNAL_UINT8(bus_data);
		decode_instruction(lcd);
	} else {
		// data
		lcd->reg_data = SIGNAL_UINT8(bus_data);
		store_data(lcd);
	}
}

static void process_end(ChipHd44780 *lcd) {

	// refresh screen if necessary
	if (PRIVATE(lcd)->refresh_screen) {
		refresh_screen(lcd);
	}

	// store state of the enable pin
	PRIVATE(lcd)->prev_enable = SIGNAL_BOOL(enable);
}

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

ChipHd44780 *chip_hd44780_create(SignalPool *signal_pool, ChipHd44780Signals signals) {
	ChipHd44780_private *priv = (ChipHd44780_private *) calloc(1, sizeof(ChipHd44780_private));
	memset(priv, 0, sizeof(ChipHd44780_private));
	ChipHd44780 *lcd = &priv->intf;

	lcd->signal_pool = signal_pool;
	memcpy(&lcd->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(bus_data, 8);
	SIGNAL_DEFINE(rs, 1);
	SIGNAL_DEFINE_BOOL(rw, 1, true);
	SIGNAL_DEFINE_BOOL(enable, 1, false);

	// perform the internal reset circuit initialization procedure
	execute_clear_display(lcd);
	execute_function_set(lcd, 1, 0, 0);
	execute_display_on_off_control(lcd, 0, 0, 0);
	execute_entry_mode_set(lcd, 1, 0);
	refresh_screen(lcd);

	return lcd;
}

void chip_hd44780_destroy(ChipHd44780 *lcd) {
	assert(lcd);
	free(PRIVATE(lcd));
}

void chip_hd44780_process(ChipHd44780 *lcd) {

	PRIVATE(lcd)->refresh_screen = false;

	// do nothing:
	//	- if not on the edge of a clock cycle
	bool enable = SIGNAL_BOOL(enable);

	if (enable == PRIVATE(lcd)->prev_enable) {
		process_end(lcd);
		return;
	}

	if (enable) {
		process_positive_enable_edge(lcd);
	} else {
		process_negative_enable_edge(lcd);
	}

	process_end(lcd);
}
