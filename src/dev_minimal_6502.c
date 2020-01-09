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
	device->sig_address = signal_create(device->signal_pool, 16);
	device->sig_data = signal_create(device->signal_pool, 8);
	device->sig_reset_b = signal_create(device->signal_pool, 1);
	device->sig_cpu_rw = signal_create(device->signal_pool, 1);
	device->sig_cpu_irq = signal_create(device->signal_pool, 1);
	device->sig_cpu_nmi = signal_create(device->signal_pool, 1);
	device->sig_cpu_sync = signal_create(device->signal_pool, 1);
	device->sig_cpu_rdy = signal_create(device->signal_pool, 1);

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
	if (rom_data) {
		memcpy(device->rom->data_array, rom_data, arrlen(rom_data));
	}

	// init data lines
	device->bus_address = 0;
	device->bus_data = 0;
	device->line_reset_b = ACTLO_ASSERT;
	device->line_cpu_rw = ACTHI_ASSERT;
	device->line_cpu_irq = ACTLO_DEASSERT;
	device->line_cpu_nmi = ACTLO_DEASSERT;
	device->line_cpu_sync = ACTHI_DEASSERT;
	device->line_cpu_rdy = ACTHI_ASSERT;

	signal_write_bool(device->signal_pool, device->sig_reset_b, ACTLO_ASSERT);
	signal_write_bool(device->signal_pool, device->sig_cpu_rw, ACTHI_ASSERT);
	signal_write_bool(device->signal_pool, device->sig_cpu_irq, ACTLO_DEASSERT);
	signal_write_bool(device->signal_pool, device->sig_cpu_nmi, ACTLO_DEASSERT);
	signal_write_bool(device->signal_pool, device->sig_cpu_sync, ACTHI_DEASSERT);
	signal_write_bool(device->signal_pool, device->sig_cpu_rdy, ACTHI_ASSERT);

	// run CPU for at least one cycle while reset is asserted
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

static inline void process_cpu(DevMinimal6502 *device, bool delayed) {
	device->bus_address = signal_read_uint16(device->signal_pool, device->sig_address);
	device->bus_data = signal_read_uint8(device->signal_pool, device->sig_data);
	device->line_reset_b = signal_read_bool(device->signal_pool, device->sig_reset_b);
	device->line_cpu_rw = signal_read_bool(device->signal_pool, device->sig_cpu_rw);
	device->line_cpu_irq = signal_read_bool(device->signal_pool, device->sig_cpu_irq);
	device->line_cpu_nmi = signal_read_bool(device->signal_pool, device->sig_cpu_nmi);
	device->line_cpu_sync = signal_read_bool(device->signal_pool, device->sig_cpu_sync);
	device->line_cpu_rdy = signal_read_bool(device->signal_pool, device->sig_reset_b);

	int8_t prev_data = device->bus_data;

	cpu_6502_process(device->cpu, delayed);

	signal_write_uint16(device->signal_pool, device->sig_address, device->bus_address);
	signal_write_bool(device->signal_pool, device->sig_cpu_rw, device->line_cpu_rw);
	signal_write_bool(device->signal_pool, device->sig_cpu_sync, device->line_cpu_sync);
	signal_write_bool(device->signal_pool, device->sig_reset_b, device->line_reset_b);

	if (prev_data != device->bus_data || !device->line_cpu_rw) {
		signal_write_uint8(device->signal_pool, device->sig_data, device->bus_data);
	}
}

static inline void process_ram(DevMinimal6502 *device) {
	device->ram->bus_address = signal_read_uint16(device->signal_pool, device->sig_address);
	device->ram->bus_data = signal_read_uint8(device->signal_pool, device->sig_data);
	device->ram->pin_ce_b = (device->ram->bus_address & 0x8000) >> 15;
	device->ram->pin_oe_b = !signal_read_bool(device->signal_pool, device->sig_cpu_rw);
	device->ram->pin_we_b = signal_read_bool(device->signal_pool, device->sig_cpu_rw) || !device->clock->pin_clock;

	ram_8d16a_process(device->ram);

	if (device->ram->upd_data) {
		signal_write_uint8(device->signal_pool, device->sig_data, device->ram->bus_data);
	}
}

static inline void process_rom(DevMinimal6502 *device) {
	device->rom->bus_address = signal_read_uint16(device->signal_pool, device->sig_address);
	device->rom->bus_data = signal_read_uint8(device->signal_pool, device->sig_data);
	device->rom->pin_ce_b = !((device->rom->bus_address & 0x8000) >> 15);

	rom_8d16a_process(device->rom);

	if (device->rom->upd_data) {
		signal_write_uint8(device->signal_pool, device->sig_data, device->rom->bus_data);
	}
}

void dev_minimal_6502_process(DevMinimal6502 *device) {
	assert(device);

	// clock tick
	clock_process(device->clock);

	///////////////////////////////////////////////////////////////////////////
	//
	// processing on the clock edge
	//

	signal_pool_cycle(device->signal_pool);
	signal_write_bool(device->signal_pool, device->sig_cpu_irq, ACTLO_DEASSERT);
	signal_write_bool(device->signal_pool, device->sig_cpu_nmi, ACTLO_DEASSERT);
	signal_write_bool(device->signal_pool, device->sig_cpu_rdy, ACTHI_ASSERT);

	// cpu
	process_cpu(device, false);

	// ram
	process_ram(device);

	// rom
	process_rom(device);


	///////////////////////////////////////////////////////////////////////////
	//
	// processing after the clock edge (i.e. after the address hold time)
	//

	signal_pool_cycle(device->signal_pool);
	signal_write_bool(device->signal_pool, device->sig_cpu_irq, ACTLO_DEASSERT);
	signal_write_bool(device->signal_pool, device->sig_cpu_nmi, ACTLO_DEASSERT);
	signal_write_bool(device->signal_pool, device->sig_cpu_rdy, ACTHI_ASSERT);

	// cpu
	process_cpu(device, true);

	// ram
	process_ram(device);

	// rom
	process_rom(device);
}

void dev_minimal_6502_reset(DevMinimal6502 *device) {

	// assert reset
	signal_write_bool(device->signal_pool, device->sig_reset_b, ACTLO_ASSERT);

	// run for a few cycles while reset is asserted
	for (int i = 0; i < 4; ++i) {
		dev_minimal_6502_process(device);
	}

	// deassert reset
	signal_write_bool(device->signal_pool, device->sig_reset_b, ACTLO_DEASSERT);

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
