// dev_minimal_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a minimal MOS-6502 based system, with 32kb of RAM and a 32kb system ROM.

#include "dev_minimal_6502.h"

#include "utils.h"
#include "stb/stb_ds.h"

#include <assert.h>
#include <stdlib.h>

DevMinimal6502 *dev_minimal_6502_create(const uint8_t *rom_data) {
	DevMinimal6502 *device = (DevMinimal6502 *) malloc(sizeof(DevMinimal6502));

	// signals
	device->signal_pool = signal_pool_create();
	device->sig_address		= signal_create(device->signal_pool, 16);
	device->sig_data		= signal_create(device->signal_pool, 8);
	device->sig_reset_b		= signal_create(device->signal_pool, 1);
	device->sig_cpu_rw		= signal_create(device->signal_pool, 1);
	device->sig_cpu_irq		= signal_create(device->signal_pool, 1);
	device->sig_cpu_nmi		= signal_create(device->signal_pool, 1);
	device->sig_cpu_sync	= signal_create(device->signal_pool, 1);
	device->sig_cpu_rdy		= signal_create(device->signal_pool, 1);

	device->sig_a15 = signal_split(device->sig_address, 15, 1);

	signal_default_bool(device->signal_pool, device->sig_cpu_irq, ACTLO_DEASSERT);
	signal_default_bool(device->signal_pool, device->sig_cpu_nmi, ACTLO_DEASSERT);
	signal_default_bool(device->signal_pool, device->sig_cpu_rdy, ACTHI_ASSERT);
	signal_default_bool(device->signal_pool, device->sig_reset_b, ACTLO_DEASSERT);

	// clock
	device->clock = clock_create(10, device->signal_pool, (ClockSignals){});

	// cpu
	device->cpu = cpu_6502_create(device->signal_pool, (Cpu6502Signals) {
										.bus_address = device->sig_address,
										.bus_data = device->sig_data,
										.clock = device->clock->signals.clock,
										.reset_b = device->sig_reset_b,
										.rw = device->sig_cpu_rw,
										.irq_b = device->sig_cpu_irq,
										.nmi_b = device->sig_cpu_nmi,
										.sync = device->sig_cpu_sync,
										.rdy = device->sig_cpu_rdy,
	});

	// ram
	device->ram = ram_8d16a_create(15, device->signal_pool, (Ram8d16aSignals) {
										.bus_address = signal_split(device->sig_address, 0, 15),
										.bus_data = device->sig_data,
										.ce_b = device->sig_a15
	});

	// rom
	device->rom = rom_8d16a_create(15, device->signal_pool, (Rom8d16aSignals) {
										.bus_address = signal_split(device->sig_address, 0, 15),
										.bus_data = device->sig_data
	});

	if (rom_data) {
		memcpy(device->rom->data_array, rom_data, arrlen(rom_data));
	}

	// run CPU for at least one cycle while reset is asserted
	signal_write_bool(device->signal_pool, device->sig_reset_b, ACTLO_ASSERT);
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
		bool next_rw = signal_read_next_bool(device->signal_pool, device->sig_cpu_rw);
		bool clock   = signal_read_next_bool(device->signal_pool, device->clock->signals.clock);

		signal_write_bool(device->signal_pool, device->ram->signals.oe_b, !next_rw);
		signal_write_bool(device->signal_pool, device->ram->signals.we_b, next_rw || !clock);

		ram_8d16a_process(device->ram);

		// rom
		//  - ce_b: assert when the top bit of the address is set
		signal_write_bool(device->signal_pool, device->rom->signals.ce_b, 
						 !signal_read_next_bool(device->signal_pool, device->sig_a15));

		rom_8d16a_process(device->rom);

		// make the clock output it's signal again
		clock_refresh(device->clock);
	}
}

void dev_minimal_6502_reset(DevMinimal6502 *device) {

	// run for a few cycles while reset is asserted
	for (int i = 0; i < 4; ++i) {
		signal_write_bool(device->signal_pool, device->sig_reset_b, ACTLO_ASSERT);
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
