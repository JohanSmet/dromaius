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

#include "chip.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
#define DDRAM_SIZE 80
#define CGRAM_SIZE 64

struct Clock;

// Normally we try to match the actual pin assignment of the chip, but the HD44780 is an 80-pin chip
// and we only expose 11 pins in the middle of the range.

typedef enum {
	// 8-bit databus
	CHIP_HD44780_DB0 = CHIP_PIN_01,
	CHIP_HD44780_DB1 = CHIP_PIN_02,
	CHIP_HD44780_DB2 = CHIP_PIN_03,
	CHIP_HD44780_DB3 = CHIP_PIN_04,
	CHIP_HD44780_DB4 = CHIP_PIN_05,
	CHIP_HD44780_DB5 = CHIP_PIN_06,
	CHIP_HD44780_DB6 = CHIP_PIN_07,
	CHIP_HD44780_DB7 = CHIP_PIN_08,

	CHIP_HD44780_RS  = CHIP_PIN_09,			// 1-bit register select
	CHIP_HD44780_RW  = CHIP_PIN_10,			// 1-bit read/write selector
	CHIP_HD44780_E   = CHIP_PIN_11,			// 1-bit enable line

} ChipHd44780SignalAssignment;

typedef Signal ChipHd44780Signals[11];

typedef struct ChipHd44780 {
	CHIP_DECLARE_FUNCTIONS

	// interface
	SignalPool *		signal_pool;
	ChipHd44780Signals	signals;

	// signal groups
	SignalGroup		sg_data;
	SignalGroup		sg_db4_7;			// upper 4-bits of the databus
	SignalGroup		sg_db0_3;			// lower 4-bits of the databus

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
ChipHd44780 *chip_hd44780_create(struct Simulator *sim, ChipHd44780Signals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CHIP_HD44780_H
