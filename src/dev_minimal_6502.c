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

	// clock
	device->clock = clock_create(10);

	// cpu
	device->cpu = cpu_6502_create(
						&device->bus_address,
						&device->bus_data,
						&device->clock->pin_clock,
						&device->line_reset_b,
						&device->line_cpu_rw,
						&device->line_cpu_irq,
						&device->line_cpu_nmi,
						&device->line_cpu_sync,
						&device->line_cpu_rdy);
	
	// ram
	device->ram = ram_8d16a_create(15);

	// rom
	device->rom = rom_8d16a_create(15);
	memcpy(device->rom->data_array, rom_data, arrlen(rom_data));

	// init data lines
	device->bus_address = 0;
	device->bus_data = 0;
	device->line_reset_b = ACTLO_ASSERT;
	device->line_cpu_rw = ACTHI_ASSERT;
	device->line_cpu_irq = ACTLO_DEASSERT;
	device->line_cpu_nmi = ACTLO_DEASSERT;
	device->line_cpu_sync = ACTHI_DEASSERT;
	device->line_cpu_rdy = ACTHI_ASSERT;

	// run CPU for at least one cycle while reset is asserted
	cpu_6502_process(device->cpu);

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
	
	// cpu
	cpu_6502_process(device->cpu);
	
	// ram
	device->ram->bus_address = device->bus_address;
	device->ram->bus_data = device->bus_data;
	device->ram->pin_ce_b = (device->bus_address & 0x8000) >> 15;
	device->ram->pin_oe_b = !device->line_cpu_rw;
	device->ram->pin_we_b = device->line_cpu_rw;

	ram_8d16a_process(device->ram);

	if (device->ram->upd_data) {
		device->bus_data = device->ram->bus_data;
	}

	// rom
	device->rom->bus_address = device->bus_address;
	device->rom->bus_data = device->bus_data;
	device->rom->pin_ce_b = !((device->bus_address & 0x8000) >> 15);

	rom_8d16a_process(device->rom);

	if (device->rom->upd_data) {
		device->bus_data = device->rom->bus_data;
	}
}

