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
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
enum DevMinimal6502SignalAssignment {

	// 16-bit address bus
	SIG_M6502_AB0 = 0,
	SIG_M6502_AB1,
	SIG_M6502_AB2,
	SIG_M6502_AB3,
	SIG_M6502_AB4,
	SIG_M6502_AB5,
	SIG_M6502_AB6,
	SIG_M6502_AB7,
	SIG_M6502_AB8,
	SIG_M6502_AB9,
	SIG_M6502_AB10,
	SIG_M6502_AB11,
	SIG_M6502_AB12,
	SIG_M6502_AB13,
	SIG_M6502_AB14,
	SIG_M6502_AB15,

	// 8-bit data bus
	SIG_M6502_DB0,
	SIG_M6502_DB1,
	SIG_M6502_DB2,
	SIG_M6502_DB3,
	SIG_M6502_DB4,
	SIG_M6502_DB5,
	SIG_M6502_DB6,
	SIG_M6502_DB7,

	SIG_M6502_RESET_BTN_B,

	SIG_M6502_RESET_B,
	SIG_M6502_CLOCK,

	SIG_M6502_CPU_RW,
	SIG_M6502_CPU_IRQ_B,
	SIG_M6502_CPU_NMI_B,
	SIG_M6502_CPU_SYNC,
	SIG_M6502_CPU_RDY,

	SIG_M6502_RAM_OE_B,
	SIG_M6502_RAM_WE_B,

	SIG_M6502_ROM_CE_B,

	SIG_M6502_PIA_CS2_B,

	SIG_M6502_LOW,
	SIG_M6502_HIGH,

	SIG_M6502_SIGNAL_COUNT
};

typedef Signal DevMinimal6502Signals[SIG_M6502_SIGNAL_COUNT];

typedef struct DevMinimal6502 {
	DEVICE_DECLARE_FUNCTIONS

	// components
	struct Cpu6502 *	cpu;
	struct Ram8d16a *	ram;
	struct Rom8d16a *	rom;
	struct Chip6520 *	pia;
	struct ChipHd44780 *lcd;
	struct InputKeypad *keypad;
	struct Oscillator *	oscillator;

	bool			in_reset;

	// signals
	SignalPool *			signal_pool;
	DevMinimal6502Signals	signals;

	SignalGroup				sg_address;
	SignalGroup				sg_data;
} DevMinimal6502;

// functions
DevMinimal6502 *dev_minimal_6502_create(const uint8_t *rom_data);
void dev_minimal_6502_destroy(DevMinimal6502 *device);
void dev_minimal_6502_reset(DevMinimal6502 *device);
void dev_minimal_6502_read_memory(DevMinimal6502 *device, size_t start_address, size_t size, uint8_t *output);
void dev_minimal_6502_write_memory(DevMinimal6502 *device, size_t start_address, size_t size, uint8_t *input);

void dev_minimal_6502_rom_from_file(DevMinimal6502 *device, const char *filename);
void dev_minimal_6502_ram_from_file(DevMinimal6502 *device, const char *filename);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_DEV_MINIMAL_6502_H
