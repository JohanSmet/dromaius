// dev_minimal_6502.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a minimal MOS-6502 based system, with 32kb of RAM and a 16kb system ROM.

/* Address space:
   0x0000 - 0x7fff (32kb) : RAM
   0x8000 - 0x800f (16)   : PIA (6520 - Peripheral Interface Adapter)
		0x8000 = DDRA/Port-A	(Data Direction Register for port-A / port-A)
		0x8001 = CRA			(Control Register for port-A)
		0x8002 = DDRB/Port-B	(Data Direction Register for port-B / port-B)
		0x8003 = CRB			(Control Register for port-B)
   0x8010 - 0xbfff        : (free)
   0xc000 - 0xffff (16kb) : ROM
*/

#ifndef DROMAIUS_DEV_MINIMAL_6502_H
#define DROMAIUS_DEV_MINIMAL_6502_H

#include "device.h"
#include "cpu_6502.h"
#include "ram_8d_16a.h"
#include "rom_8d_16a.h"
#include "chip_6520.h"
#include "chip_hd44780.h"
#include "input_keypad.h"
#include "clock.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct DevMinimal6502Signals {
	Signal		bus_address;		// 16-bit address bus
	Signal		bus_data;			// 8-bit data bus

	Signal		reset_b;			// 1-bit
	Signal		clock;				// 1-bit

	Signal		cpu_rw;				// 1-bit
	Signal		cpu_irq_b;			// 1-bit
	Signal		cpu_nmi_b;			// 1-bit
	Signal		cpu_sync;			// 1-bit
	Signal		cpu_rdy;			// 1-bit

	Signal		ram_oe_b;			// 1-bit
	Signal		ram_we_b;			// 1-bit

	Signal		rom_ce_b;			// 1-bit

	Signal		pia_cs2_b;			// 1-bit

	Signal		a15;				// 1-bit - top bit of the address bus
	Signal		a14;				// 1-bit - second most significat bit of the address bus

	Signal		high;				// 1-bit - always high
	Signal		low;				// 1-bit - always low
} DevMinimal6502Signals;

typedef struct DevMinimal6502 {
	DEVICE_DECLARE_FUNCTIONS

	// components
	Cpu6502 *		cpu;
	Ram8d16a *		ram;
	Rom8d16a *		rom;
	Chip6520 *		pia;
	ChipHd44780 *	lcd;
	InputKeypad *	keypad;
	struct Oscillator *oscillator;

	bool			in_reset;

	// signals
	DevMinimal6502Signals	signals;
} DevMinimal6502;

// functions
DevMinimal6502 *dev_minimal_6502_create(const uint8_t *rom_data);
void dev_minimal_6502_destroy(DevMinimal6502 *device);
void dev_minimal_6502_process(DevMinimal6502 *device);
void dev_minimal_6502_reset(DevMinimal6502 *device);
void dev_minimal_6502_copy_memory(DevMinimal6502 *device, size_t start_address, size_t size, uint8_t *output);

void dev_minimal_6502_rom_from_file(DevMinimal6502 *device, const char *filename);
void dev_minimal_6502_ram_from_file(DevMinimal6502 *device, const char *filename);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_DEV_MINIMAL_6502_H
