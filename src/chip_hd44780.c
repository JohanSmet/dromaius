// chip_hd44780.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Partial emulation of the HD44780 Dot Matrix LCD Controller/Driver

#include "chip_hd44780.h"
#include "clock.h"

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

typedef enum DataCycle {
	DC_4BIT_HI = 0,
	DC_4BIT_LO = 1,
	DC_8BIT = 2
} DataCycle;

typedef struct ChipHd44780_private {
	ChipHd44780		intf;
	Clock *			clock;

	uint8_t			ddram_addr;				// continuous 0 - 79 even for two line displays (no gap between 1st & 2nd line)
	uint8_t			cgram_mask;				// address mask for the cgram character index

	bool			prev_enable;
	int8_t			address_delta;			// increment or decrement on read/write
	RamMode			ram_mode;				// accessing DDRAM or CGRAM ?

	DataLen			data_len;				// width of the MPU interface
	DataCycle		data_cycle;				// to distinguish the first and second nibble of a 4-bit MPU transfer
	uint8_t			data_in;				// temp-storage for incoming data in 4-bit mode

	bool			refresh_screen;			// should the dot-matrix ouput be refreshed at the end of the processing function?

	bool			shift_enabled;			// display shift applied?
	int				shift_delta;			// number of characters to shift

	bool			cursor_enabled;			// cursor enable
	bool			cursor_blink;			// blinking cursor enabled (required cursor_enabled)
	bool			cursor_block;			// current phase of the blinking cursor (full block or bottom line only)
	int64_t			cursor_time;			// last time phase of blinking cursor was changed
} ChipHd44780_private;

static const uint8_t rom_a00[256][16] = {
	#include "chip_hd44780_a00.inc"
};

//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#define PRIVATE(lcd)	((ChipHd44780_private *) lcd)
#define RW_READ			true
#define RW_WRITE		false

static inline bool output_register_to_databus(ChipHd44780 *lcd, bool final) {
	// when rs = high: read data from CGRAM or DDRAM via the data register
	// when rs = low:  read busy-flag and address (note: busy flag not implemented)
	uint8_t data = SIGNAL_BOOL(rs) ? lcd->reg_data : lcd->reg_ac & 0x7f;

	switch (PRIVATE(lcd)->data_cycle) {
		case DC_4BIT_HI :
			SIGNAL_SET_UINT8(db4_7, (data & 0xf0) >> 4);
			PRIVATE(lcd)->data_cycle += final;
			return false;

		case DC_4BIT_LO :
			SIGNAL_SET_UINT8(db4_7, (data & 0x0f));
			PRIVATE(lcd)->data_cycle -= final;
			return true;

		case DC_8BIT :
			SIGNAL_SET_UINT8(db4_7, (data & 0xf0) >> 4);
			SIGNAL_SET_UINT8(db0_3, (data & 0x0f));
			return true;

		default :
			return false;
	}
}

static inline bool input_from_databus(ChipHd44780 *lcd) {
	switch (PRIVATE(lcd)->data_cycle) {
		case DC_4BIT_HI :
			PRIVATE(lcd)->data_in = (uint8_t) (SIGNAL_UINT8(db4_7) << 4);
			PRIVATE(lcd)->data_cycle = DC_4BIT_LO;
			return false;

		case DC_4BIT_LO :
			PRIVATE(lcd)->data_in = (uint8_t) (PRIVATE(lcd)->data_in | (SIGNAL_UINT8(db4_7) & 0xf));
			PRIVATE(lcd)->data_cycle = DC_4BIT_HI;
			return true;

		case DC_8BIT :
			PRIVATE(lcd)->data_in = (uint8_t) ((SIGNAL_UINT8(db4_7) << 4) | SIGNAL_UINT8(db0_3));
			return true;

		default :
			return false;
	}
}

static inline uint8_t ddram_virtual_to_physical(ChipHd44780 *lcd, int addr) {
	if (lcd->display_height == 1) {
		return (uint8_t) addr;
	}

	if (addr >= 64) {
		// there's a 'virtual' gap between the first and second line
		return (uint8_t) (addr - 24);
	} else if (addr >= 40) {
		// round up when in the gap between 1st and 2nd line
		return 64;
	} else {
		return (uint8_t) addr;
	}
}

static inline uint8_t ddram_physical_to_virtual(ChipHd44780 *lcd, uint8_t addr) {
	if (lcd->display_height == 1) {
		return addr;
	}

	return (uint8_t) ((addr >= 40) ? addr + 24 : addr);
}

static inline uint8_t ddram_valid_virtual_address(ChipHd44780 *lcd, int addr) {
	if (lcd->display_height == 1) {
		return (uint8_t) ((addr + 80) % 80);
	} else {
		// first line from 0-40 (0x00-0x27) / second line from 64-103 (0x40-0x67)
		uint8_t result = (uint8_t) ((addr + 104) % 104);
		return (result >= 40 && result < 64) ? 64 : result;
	}
}

static inline uint8_t ddram_valid_physical_address(int addr) {
	return (uint8_t)((addr + 80) % 80);
}

static inline uint8_t cgram_valid_address(int addr) {
	 return addr & 0x3f;
}

static inline void ddram_set_address(ChipHd44780 *lcd, uint8_t address) {
	lcd->reg_ac = ddram_valid_virtual_address(lcd, address);

	PRIVATE(lcd)->ddram_addr = ddram_virtual_to_physical(lcd, lcd->reg_ac);
	lcd->reg_data = lcd->ddram[PRIVATE(lcd)->ddram_addr];
	PRIVATE(lcd)->ram_mode = RM_DDRAM;
}

static inline void cgram_set_address(ChipHd44780 *lcd, uint8_t address) {
	lcd->reg_ac = cgram_valid_address(address);
	lcd->reg_data = lcd->cgram[lcd->reg_ac];
	PRIVATE(lcd)->ram_mode = RM_CGRAM;
}

static inline void increment_decrement_addres(ChipHd44780 *lcd) {

	switch (PRIVATE(lcd)->ram_mode) {
		case RM_DDRAM:
			PRIVATE(lcd)->ddram_addr = ddram_valid_physical_address(PRIVATE(lcd)->ddram_addr + PRIVATE(lcd)->address_delta);
			lcd->reg_ac = ddram_physical_to_virtual(lcd, PRIVATE(lcd)->ddram_addr);
			lcd->reg_data = lcd->ddram[PRIVATE(lcd)->ddram_addr];
			break;
		case RM_CGRAM:
			lcd->reg_ac = cgram_valid_address(lcd->reg_ac + PRIVATE(lcd)->address_delta);
			lcd->reg_data = lcd->cgram[lcd->reg_ac];
			break;
	}
}

static inline void execute_clear_display(ChipHd44780 *lcd) {
	// - write 0x20 into all DDRAM address
	// - set DDRAM address 0 into the address counter
	// - return display to original status if it was shifted
	// - set I/D of entry mode to 1 (increment) / S doesn't change
	memset(lcd->ddram, 0x20, DDRAM_SIZE);
	memset(lcd->display_data, 0, arrlenu(lcd->display_data));
	ddram_set_address(lcd, 0);
	PRIVATE(lcd)->shift_delta = 0;
	PRIVATE(lcd)->address_delta = 1;
	PRIVATE(lcd)->refresh_screen = true;
}

static inline void execute_return_home(ChipHd44780 *lcd) {
	// - set DDRAM address 0 into the address counter
	// - return display to original status if it was shifted
	ddram_set_address(lcd, 0);
	PRIVATE(lcd)->shift_delta = 0;
	PRIVATE(lcd)->refresh_screen = true;
}

static inline void execute_entry_mode_set(ChipHd44780 *lcd, bool inc_or_dec, bool shift) {
	PRIVATE(lcd)->address_delta = (inc_or_dec) ? 1 : -1;
	PRIVATE(lcd)->shift_enabled = shift;
}

static inline void execute_display_on_off_control(ChipHd44780 *lcd, bool display, bool cursor, bool cursor_blink) {
	lcd->display_enabled = display;
	PRIVATE(lcd)->cursor_enabled = cursor;
	PRIVATE(lcd)->cursor_blink = cursor_blink;
	PRIVATE(lcd)->refresh_screen = true;
}

static inline void execute_cursor_or_display_shift(ChipHd44780 *lcd, bool display_or_cursor, bool right_or_left) {
	if (display_or_cursor) {
		// display shift
		if (PRIVATE(lcd)->shift_enabled) {
			PRIVATE(lcd)->shift_delta += (right_or_left) ? -1 : 1;
			PRIVATE(lcd)->shift_delta = (((int16_t) PRIVATE(lcd)->shift_delta + 80) % 160) - 80;
		}
	} else if (PRIVATE(lcd)->ram_mode == RM_DDRAM) {
		// cursor shift
		PRIVATE(lcd)->ddram_addr = ddram_valid_physical_address(PRIVATE(lcd)->ddram_addr + (right_or_left ? 1 : -1));
		lcd->reg_ac = ddram_physical_to_virtual(lcd, PRIVATE(lcd)->ddram_addr);
	}
	PRIVATE(lcd)->refresh_screen = true;
}

static inline void execute_function_set(ChipHd44780 *lcd, bool dl, bool n, bool f) {
	// dl = 1: 8-bits, dl = 0: 4-bits
	//  n = 1: 2 lines, n = 0: 1 line
	//  f = 1: 5 x 10 dots, f = 0: 5 x 8 dots
	PRIVATE(lcd)->data_len = (dl) ? DL_8BIT : DL_4BIT;
	PRIVATE(lcd)->data_cycle = (dl) ? DC_8BIT : DC_4BIT_HI;

	lcd->display_width = 16;
	lcd->display_height = (n) ? 2 : 1;
	lcd->char_width = 5;
	lcd->char_height = (f) ? 10 : 8;
	arrsetlen(lcd->display_data, (size_t) (lcd->display_width * lcd->display_height * lcd->char_width * lcd->char_height));

	PRIVATE(lcd)->refresh_screen = true;
	PRIVATE(lcd)->cgram_mask = (f) ? 0x03 : 0x07;
}

static inline void execute_set_cgram_address(ChipHd44780 *lcd, uint8_t addr) {
	cgram_set_address(lcd, addr);
}

static inline void execute_set_ddram_address(ChipHd44780 *lcd, uint8_t addr) {
	ddram_set_address(lcd, addr);
	PRIVATE(lcd)->refresh_screen = true;
}

static inline void decode_instruction(ChipHd44780 *lcd) {
	if (BIT_IS_SET(lcd->reg_ir, 7)) {
		execute_set_ddram_address(lcd, lcd->reg_ir & 0x7f);
	} else if (BIT_IS_SET(lcd->reg_ir, 6)) {
		execute_set_cgram_address(lcd, lcd->reg_ir & 0x3f);
	} else if (BIT_IS_SET(lcd->reg_ir, 5)) {
		execute_function_set(lcd,	BIT_IS_SET(lcd->reg_ir, 4),
									BIT_IS_SET(lcd->reg_ir, 3),
									BIT_IS_SET(lcd->reg_ir, 2));
	} else if (BIT_IS_SET(lcd->reg_ir, 4)) {
		execute_cursor_or_display_shift(lcd,	BIT_IS_SET(lcd->reg_ir, 3),
												BIT_IS_SET(lcd->reg_ir, 2));
	} else if (BIT_IS_SET(lcd->reg_ir, 3)) {
		execute_display_on_off_control(lcd, BIT_IS_SET(lcd->reg_ir, 2),
											BIT_IS_SET(lcd->reg_ir, 1),
											BIT_IS_SET(lcd->reg_ir, 0));
	} else if (BIT_IS_SET(lcd->reg_ir, 2)) {
		execute_entry_mode_set(lcd, BIT_IS_SET(lcd->reg_ir, 1),
									BIT_IS_SET(lcd->reg_ir, 0));
	} else if (BIT_IS_SET(lcd->reg_ir, 1)) {
		execute_return_home(lcd);
	} else if (BIT_IS_SET(lcd->reg_ir, 0)) {
		execute_clear_display(lcd);
	}
}

static inline void store_data(ChipHd44780 *lcd) {
	switch (PRIVATE(lcd)->ram_mode) {
		case RM_CGRAM:
			lcd->cgram[lcd->reg_ac] = lcd->reg_data;
			break;
		case RM_DDRAM:
			lcd->ddram[ddram_virtual_to_physical(lcd, lcd->reg_ac)] = lcd->reg_data;
			PRIVATE(lcd)->refresh_screen = true;
			break;
	}

	increment_decrement_addres(lcd);
}

static inline void draw_character(ChipHd44780 *lcd, uint8_t c, int x, int y) {
	const uint8_t *src = rom_a00[c];
	if ((c & 0xf0) == 0) {
		// get data from cgram instead of rom
		src = lcd->cgram + ((c & PRIVATE(lcd)->cgram_mask) * lcd->char_height);
	}
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

	int limit = 80 / lcd->display_height;

	// characters
	for (int l = 0; l < lcd->display_height; ++l) {
		for (int i = 0; i < 16; ++i) {
			int pos = (i + PRIVATE(lcd)->shift_delta + limit) % limit;
			uint8_t c = lcd->ddram[ddram_virtual_to_physical(lcd, (0x40 * l) + pos)];
			draw_character(lcd, c, i, l);
		}
	}

	// cursor
	if (PRIVATE(lcd)->cursor_enabled && PRIVATE(lcd)->ram_mode == RM_DDRAM) {
		int cursor_x = lcd->reg_ac;
		int cursor_y = 0;
		if (lcd->display_height == 2 && cursor_x >= 0x40) {
			cursor_x -= 0x40;
			cursor_y = 1;
		}
		cursor_x -= PRIVATE(lcd)->shift_delta;

		if (cursor_x >= 0 && cursor_x < lcd->display_width)  {
			int line_stride = lcd->char_width * lcd->display_width;
			int first_line = lcd->char_height - 1;
			int num_lines = 1;
			if (PRIVATE(lcd)->cursor_blink && PRIVATE(lcd)->cursor_block) {
				first_line = 0;
				num_lines = lcd->char_height;
			}

			uint8_t *dst = lcd->display_data + ((cursor_y * lcd->char_height + first_line) * line_stride) +
												(cursor_x * lcd->char_width);

			for (int l = 0; l < num_lines; ++l) {
				for (int i = 0; i < 5; ++i) {
					dst[i] = 1;
				}
				dst += line_stride;
			}
		}
	}
}

static inline void process_positive_enable_edge(ChipHd44780 *lcd) {
	if (SIGNAL_BOOL(rw) == RW_READ) {
		output_register_to_databus(lcd, false);
	}
}

static inline void process_negative_enable_edge(ChipHd44780 *lcd) {
	// read mode
	if (SIGNAL_BOOL(rw) == RW_READ) {
		if (output_register_to_databus(lcd, true)) {
			increment_decrement_addres(lcd);
		}
		return;
	}

	// write mode
	if (input_from_databus(lcd)) {
		if (SIGNAL_BOOL(rs) == false) {
			// instruction
			lcd->reg_ir = PRIVATE(lcd)->data_in;
			decode_instruction(lcd);
		} else {
			// data
			lcd->reg_data = PRIVATE(lcd)->data_in;
			store_data(lcd);
		}
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

ChipHd44780 *chip_hd44780_create(struct Clock *clock, SignalPool *signal_pool, ChipHd44780Signals signals) {
	assert(signals.bus_data.count == 0 || (signals.bus_data.count > 0 && signals.db4_7.count == 0 && signals.db0_3.count == 0));

	ChipHd44780_private *priv = (ChipHd44780_private *) calloc(1, sizeof(ChipHd44780_private));
	memset(priv, 0, sizeof(ChipHd44780_private));
	priv->clock = clock;
	ChipHd44780 *lcd = &priv->intf;
	CHIP_SET_FUNCTIONS(lcd, chip_hd44780_process, chip_hd44780_destroy, chip_hd44780_register_dependencies);

	lcd->signal_pool = signal_pool;
	memcpy(&lcd->signals, &signals, sizeof(signals));

	// the databus is setup differently than other modules. In 4-bit interface mode only the upper nibble is used.
	// We try to set it up so writing to signal 'bus_data' works in 8-bit mode but also allow a 4-bit signal to be
	// connected to the upper nibble when in 4-bit mode. Even in 8-bit mode we always use the two 4-bit signals.
	// The LCD always starts in 8-bit mode and this way it can be switched to 4-bt mode with only the top 4-bits connected.
	if (SIGNAL(bus_data).count == 0 && SIGNAL(db0_3).count == 0 && SIGNAL(db4_7).count == 0) {
		SIGNAL_DEFINE(bus_data, 8);
	}

	if (SIGNAL(bus_data).count > 0) {
		// NOTE: this assumes that whatever connects to the databus is at least 8 bits wide
		lcd->signals.db0_3 = signal_split(SIGNAL(bus_data), 0, 4);
		lcd->signals.db4_7 = signal_split(SIGNAL(bus_data), 4, 4);
	} else {
		SIGNAL_DEFINE(db0_3, 4);
		SIGNAL_DEFINE(db4_7, 4);
	}

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

void chip_hd44780_register_dependencies(ChipHd44780 *lcd) {
	assert(lcd);
	signal_add_dependency(lcd->signal_pool, SIGNAL(enable), lcd->id);
}

void chip_hd44780_destroy(ChipHd44780 *lcd) {
	assert(lcd);
	free(PRIVATE(lcd));
}

void chip_hd44780_process(ChipHd44780 *lcd) {

	PRIVATE(lcd)->refresh_screen = false;

	// cursor blink - this is a little sketchy
	//	datasheet states the cursor blinks at a speed of 409.6 ms intervals when the input frequency is standard
	//	we just assume this function gets called twice for each clock change (on and after the change) so the time passed is
	//  a quarter of the clock period. This should work reasonably well when stepping through the executable
	static const int64_t BLINK_INTERVAL = 410 * 1000000 * 2;
	if (PRIVATE(lcd)->cursor_enabled && PRIVATE(lcd)->cursor_blink && PRIVATE(lcd)->clock) {
		PRIVATE(lcd)->cursor_time += PRIVATE(lcd)->clock->conf_half_period_ns;
		if (PRIVATE(lcd)->cursor_time > BLINK_INTERVAL) {
			PRIVATE(lcd)->cursor_block = !PRIVATE(lcd)->cursor_block;
			PRIVATE(lcd)->refresh_screen = true;
			PRIVATE(lcd)->cursor_time -= BLINK_INTERVAL;
		}
	}

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
