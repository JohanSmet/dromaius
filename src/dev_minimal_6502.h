// dev_minimal_6502.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a minimal MOS-6502 based system, with 32kb of RAM and a 32kb system ROM.

#ifndef DROMAIUS_DEV_MINIMAL_6502_H
#define DROMAIUS_DEV_MINIMAL_6502_H

#include "cpu_6502.h"
#include "ram_8d_16a.h"
#include "rom_8d_16a.h"
#include "clock.h"

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

	Signal		a15;				// 1-bit - top bit of the address bus
} DevMinimal6502Signals;

typedef struct DevMinimal6502 {
	// components
	Cpu6502 *	cpu;
	Ram8d16a *	ram;
	Rom8d16a *	rom;
	Clock *		clock;

	// signals
	SignalPool *			signal_pool;
	DevMinimal6502Signals	signals;
} DevMinimal6502;

// functions
DevMinimal6502 *dev_minimal_6502_create(const uint8_t *rom_data);
void dev_minimal_6502_destroy(DevMinimal6502 *device);
void dev_minimal_6502_process(DevMinimal6502 *device);
void dev_minimal_6502_reset(DevMinimal6502 *device);

void dev_minimal_6502_rom_from_file(DevMinimal6502 *device, const char *filename);
void dev_minimal_6502_ram_from_file(DevMinimal6502 *device, const char *filename);

#endif // DROMAIUS_DEV_MINIMAL_6502_H
