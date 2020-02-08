// chip_hd44780.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Partial emulation of the HD44780 Dot Matrix LCD Controller/Driver
//
// Most features should be supported, although this was written with only the Hitachi HD44780U datasheet as a reference.
// I haven't compared the functionality with real hardware (yet), simply because I don't own one ATM.
// Overview of stuff that isn't implemented:
//	- the busy flag - all operations complete on the negative edge of the enable signal 
//	- only the A00 character rom is included
//	- blinking cursor: timing is approximated
//  - "limited" to 16x1 or 16x2 LCD output

#ifndef DROMAIUS_CHIP_HD44780_H
#define DROMAIUS_CHIP_HD44780_H

#include "types.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
#define DDRAM_SIZE 80
#define CGRAM_SIZE 64

struct Clock;

typedef struct ChipHd44780Signals {
	Signal	bus_data;		// 8-bit data bus
	Signal	rs;				// 1-bit register select
	Signal	rw;				// 1-bit read/write selector
	Signal  enable;			// 1-bit enable line

	Signal	db4_7;			// upper 4-bits of the databus
	Signal	db0_3;			// lower 4-bits of the databus
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
ChipHd44780 *chip_hd44780_create(struct Clock *clock, SignalPool *signal_pool, ChipHd44780Signals signals);
void chip_hd44780_destroy(ChipHd44780 *lcd);
void chip_hd44780_process(ChipHd44780 *lcd);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_HD44780_H
