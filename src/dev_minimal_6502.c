// dev_minimal_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a minimal MOS-6502 based system, with 32kb of RAM and a 16kb system ROM.

#include "dev_minimal_6502.h"

#include "utils.h"
#include "stb/stb_ds.h"

#include <assert.h>
#include <stdlib.h>

#define SIGNAL_POOL			device->signal_pool
#define SIGNAL_COLLECTION	device->signals

DevMinimal6502 *dev_minimal_6502_create(const uint8_t *rom_data) {
	DevMinimal6502 *device = (DevMinimal6502 *) calloc(1, sizeof(DevMinimal6502));

	// signals
	device->signal_pool = signal_pool_create();

	SIGNAL_DEFINE(bus_address, 16);
	SIGNAL_DEFINE(bus_data, 8);
	SIGNAL_DEFINE_BOOL(clock, 1, true);
	SIGNAL_DEFINE_BOOL(reset_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE(cpu_rw, 1);
	SIGNAL_DEFINE_BOOL(cpu_irq_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(cpu_nmi_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE(cpu_sync, 1);
	SIGNAL_DEFINE_BOOL(cpu_rdy, 1, ACTHI_ASSERT);

	device->signals.a15 = signal_split(SIGNAL(bus_address), 15, 1);
	device->signals.a14 = signal_split(SIGNAL(bus_address), 14, 1);

	// clock
	device->clock = clock_create(10, device->signal_pool, (ClockSignals){
										.clock = SIGNAL(clock)
	});

	// cpu
	device->cpu = cpu_6502_create(device->signal_pool, (Cpu6502Signals) {
										.bus_address = SIGNAL(bus_address),
										.bus_data = SIGNAL(bus_data),
										.clock = SIGNAL(clock),
										.reset_b = SIGNAL(reset_b),
										.rw = SIGNAL(cpu_rw),
										.irq_b = SIGNAL(cpu_irq_b),
										.nmi_b = SIGNAL(cpu_nmi_b),
										.sync = SIGNAL(cpu_sync),
										.rdy = SIGNAL(cpu_rdy)
	});

	// ram
	device->ram = ram_8d16a_create(15, device->signal_pool, (Ram8d16aSignals) {
										.bus_address = signal_split(device->signals.bus_address, 0, 15),
										.bus_data = SIGNAL(bus_data),
										.ce_b = SIGNAL(a15)
	});

	// rom
	device->rom = rom_8d16a_create(14, device->signal_pool, (Rom8d16aSignals) {
										.bus_address = signal_split(device->signals.bus_address, 0, 14),
										.bus_data = SIGNAL(bus_data),
	});

	if (rom_data) {
		memcpy(device->rom->data_array, rom_data, arrlen(rom_data));
	}

	// copy some signals for easy access
	SIGNAL(ram_oe_b) = device->ram->signals.oe_b;
	SIGNAL(ram_we_b) = device->ram->signals.we_b;
	SIGNAL(rom_ce_b) = device->rom->signals.ce_b;

	// run CPU for at least one cycle while reset is asserted
	signal_write_bool(device->signal_pool, SIGNAL(reset_b), ACTLO_ASSERT);
	cpu_6502_process(device->cpu, false);

	return device;
}

void dev_minimal_6502_destroy(DevMinimal6502 *device) {
	assert(device);

	clock_destroy(device->clock);
	cpu_6502_destroy(device->cpu);
	ram_8d16a_destroy(device->ram);
	rom_8d16a_destroy(device->rom);
	free(device);
}

void dev_minimal_6502_process(DevMinimal6502 *device) {
	assert(device);

	// clock tick
	clock_process(device->clock);

	// run process twice, once at the time of the clock edge and once slightly later (after the address hold time)
	for (int time = 0; time < 2; ++time) {

		signal_pool_cycle(device->signal_pool);

		// cpu
		cpu_6502_process(device->cpu, time & 1);

		// ram
		//  - ce_b: assert when top bit of address isn't set (copy of a15)
		//	- oe_b: assert when cpu_rw is high
		//	- we_b: assert when cpu_rw is low and clock is low
		bool next_rw = SIGNAL_NEXT_BOOL(cpu_rw);

		SIGNAL_SET_BOOL(ram_oe_b, !next_rw);
		SIGNAL_SET_BOOL(ram_we_b, next_rw || !SIGNAL_NEXT_BOOL(clock));

		ram_8d16a_process(device->ram);

		// rom
		//  - ce_b: assert when the top 2 bits of the address is set
		SIGNAL_SET_BOOL(rom_ce_b, !(SIGNAL_NEXT_BOOL(a15) & SIGNAL_NEXT_BOOL(a14)));

		rom_8d16a_process(device->rom);

		// make the clock output it's signal again
		clock_refresh(device->clock);
	}
}

void dev_minimal_6502_reset(DevMinimal6502 *device) {

	// run for a few cycles while reset is asserted
	for (int i = 0; i < 4; ++i) {
		SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
		dev_minimal_6502_process(device);
	}

	// run CPU init cycle
	for (int i = 0; i < 15; ++i) {
		dev_minimal_6502_process(device);
	}

	// reset clock
	device->clock->cycle_count = 0;
}

void dev_minimal_6502_rom_from_file(DevMinimal6502 *device, const char *filename) {
	file_load_binary_fixed(filename, device->rom->data_array, device->rom->data_size);
}

void dev_minimal_6502_ram_from_file(DevMinimal6502 *device, const char *filename) {
	file_load_binary_fixed(filename, device->ram->data_array, device->ram->data_size);
}
