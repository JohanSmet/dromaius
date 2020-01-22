// chip_hd44780.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Partial emulation of the HD44780 Dot Matrix LCD Controller/Driver

#ifndef DROMAIUS_CHIP_HD44780_H
#define DROMAIUS_CHIP_HD44780_H

#include "types.h"
#include "signal_line.h"

// types
#define DDRAM_SIZE 80
#define CGRAM_SIZE 64

typedef struct ChipHd44780Signals {
	Signal	bus_data;		// 8-bit data bus
	Signal	rs;				// 1-bit register select
	Signal	rw;				// 1-bit read/write selector
	Signal  enable;			// 1-bit enable line
} ChipHd44780Signals;

typedef struct ChipHd44780 {
	// interface
	SignalPool *		signal_pool;
	ChipHd44780Signals	signals;

	// registers
	uint8_t		reg_ir;				// 8-bit instruction register
	uint8_t		reg_data;			// 8-bit data register
	uint8_t		reg_ac;				// 8-bit address counter

	uint8_t		ddram[DDRAM_SIZE];	// 80 x 8-bit Display data RAM
	uint8_t		cgram[CGRAM_SIZE];	// 64 bytes Character generator RAM

	// output properties
	bool		display_enabled;
	uint8_t		display_width;		// in characters
	uint8_t		display_height;		// in characters
	uint8_t		char_width;			// width of one character cell (in pixels)
	uint8_t		char_height;		// height of one character cell (in pixels);
	uint8_t *	display_data;
} ChipHd44780;

// functions
ChipHd44780 *chip_hd44780_create(SignalPool *signal_pool, ChipHd44780Signals signals);
void chip_hd44780_destroy(ChipHd44780 *lcd);
void chip_hd44780_process(ChipHd44780 *lcd);

#endif // DROMAIUS_CHIP_HD44780_H
