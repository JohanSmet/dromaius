// dev_minimal_6502.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a minimal MOS-6502 based system, with 32kb of RAM and a 32kb system ROM.

#ifndef DROMAIUS_DEV_MINIMAL_6502_H
#define DROMAIUS_DEV_MINIMAL_6502_H

#include "signal.h"
#include "cpu_6502.h"
#include "ram_8d_16a.h"
#include "rom_8d_16a.h"
#include "clock.h"

// types 
typedef struct DevMinimal6502 {
	// components
	SignalPool *signal_pool;
	Cpu6502 *	cpu;
	Ram8d16a *	ram;
	Rom8d16a *	rom;
	Clock *		clock;

	// data lines
	uint16_t	bus_address;
	uint8_t		bus_data;
	bool		line_reset_b;
	bool		line_cpu_rw;
	bool		line_cpu_irq;
	bool		line_cpu_nmi;
	bool		line_cpu_sync;
	bool		line_cpu_rdy;

	// signals
	Signal		sig_address;		// 16-bit
	Signal		sig_data;			// 8-bit
	Signal		sig_reset_b;		// 1-bit
	Signal		sig_cpu_rw;			// 1-bit
	Signal		sig_cpu_irq;		// 1-bit
	Signal		sig_cpu_nmi;		// 1-bit
	Signal		sig_cpu_sync;		// 1-bit
	Signal		sig_cpu_rdy;		// 1-bit
} DevMinimal6502;

// functions
DevMinimal6502 *dev_minimal_6502_create(const uint8_t *rom_data);
void dev_minimal_6502_destroy(DevMinimal6502 *device);
void dev_minimal_6502_process(DevMinimal6502 *device);
void dev_minimal_6502_reset(DevMinimal6502 *device);

void dev_minimal_6502_rom_from_file(DevMinimal6502 *device, const char *filename);
void dev_minimal_6502_ram_from_file(DevMinimal6502 *device, const char *filename);

#endif // DROMAIUS_DEV_MINIMAL_6502_H
