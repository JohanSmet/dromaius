// dev_minimal_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a minimal MOS-6502 based system, with 32kb of RAM and a 16kb system ROM.

#include "dev_minimal_6502.h"

#include "utils.h"
#include "stb/stb_ds.h"

#include "chip_poweronreset.h"
#include "chip_oscillator.h"

#include <assert.h>
#include <stdlib.h>

#define SIGNAL_POOL			device->signal_pool
#define SIGNAL_COLLECTION	device->signals
#define SIGNAL_CHIP_ID		chip->id

///////////////////////////////////////////////////////////////////////////////
//
// internal types / functions
//

typedef struct ChipGlueLogic {
	CHIP_DECLARE_FUNCTIONS

	DevMinimal6502 *device;
} ChipGlueLogic;

static void glue_logic_register_dependencies(ChipGlueLogic *chip);
static void glue_logic_process(ChipGlueLogic *chip);
static void glue_logic_destroy(ChipGlueLogic *chip);

static ChipGlueLogic *glue_logic_create(DevMinimal6502 *device) {
	ChipGlueLogic *chip = (ChipGlueLogic *) calloc(1, sizeof(ChipGlueLogic));
	chip->device = device;

	CHIP_SET_FUNCTIONS(chip, glue_logic_process, glue_logic_destroy, glue_logic_register_dependencies);

	return chip;
}

static void glue_logic_register_dependencies(ChipGlueLogic *chip) {
	assert(chip);
	DevMinimal6502 *device = chip->device;

	signal_add_dependency(device->signal_pool, SIGNAL(cpu_rw), chip->id);
	signal_add_dependency(device->signal_pool, SIGNAL(clock), chip->id);
	signal_add_dependency(device->signal_pool, SIGNAL(bus_address), chip->id);
}

static void glue_logic_destroy(ChipGlueLogic *chip) {
	assert(chip);
	free(chip);
}

static void glue_logic_process(ChipGlueLogic *chip) {
	assert(chip);
	DevMinimal6502 *device = chip->device;

	// >> ram logic
	//  - ce_b: assert when top bit of address isn't set (copy of a15)
	//	- oe_b: assert when cpu_rw is high
	//	- we_b: assert when cpu_rw is low and clock is high
	bool next_rw = SIGNAL_BOOL(cpu_rw);
	SIGNAL_SET_BOOL(ram_oe_b, !next_rw);
	SIGNAL_SET_BOOL(ram_we_b, next_rw || !SIGNAL_BOOL(clock));

	// >> rom logic
	//  - ce_b: assert when the top 2 bits of the address is set
	SIGNAL_SET_BOOL(rom_ce_b, !(SIGNAL_BOOL(a15) & SIGNAL_BOOL(a14)));

	// >> pia logic
	//  - no peripheral connected, irq lines not connected
	//	- cs0: assert when top bit of address is set (copy of a15)
	//	- cs1: always asserted
	//  - cs2_b: assert when bits 4-14 are zero
	uint16_t bus_address = SIGNAL_UINT16(bus_address);
	SIGNAL_SET_BOOL(pia_cs2_b, (bus_address & 0x7ff0) != 0x0000);
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

#if 0

static inline void activate_reset(DevMinimal6502 *device, bool reset) {
	device->in_reset = reset;
	SIGNAL_SET_BOOL(reset_b, !device->in_reset);
}

#endif

Cpu6502* dev_minimal_6502_get_cpu(DevMinimal6502 *device) {
	assert(device);
	return device->cpu;
}

DevMinimal6502 *dev_minimal_6502_create(const uint8_t *rom_data) {
	DevMinimal6502 *device = (DevMinimal6502 *) calloc(1, sizeof(DevMinimal6502));

	device->get_cpu = (DEVICE_GET_CPU) dev_minimal_6502_get_cpu;
	device->process = (DEVICE_PROCESS) dev_minimal_6502_process;
	device->reset = (DEVICE_RESET) dev_minimal_6502_reset;
	device->destroy = (DEVICE_DESTROY) dev_minimal_6502_destroy;

	// signals
	device->signal_pool = signal_pool_create(1);
	device->signal_pool->tick_duration_ps = 20000;		// 20ns

	SIGNAL_DEFINE(bus_address, 16);
	SIGNAL_DEFINE(bus_data, 8);
	SIGNAL_DEFINE_BOOL(clock, 1, true);
	SIGNAL_DEFINE_BOOL(reset_b, 1, ACTLO_ASSERT);
	SIGNAL_DEFINE_BOOL(cpu_rw, 1, true);
	SIGNAL_DEFINE_BOOL(cpu_irq_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(cpu_nmi_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE(cpu_sync, 1);
	SIGNAL_DEFINE_BOOL(cpu_rdy, 1, ACTHI_ASSERT);

	SIGNAL_DEFINE_BOOL(low, 1, false);
	SIGNAL_DEFINE_BOOL(high, 1, true);

	device->signals.a15 = signal_split(SIGNAL(bus_address), 15, 1);
	device->signals.a14 = signal_split(SIGNAL(bus_address), 14, 1);

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
	DEVICE_REGISTER_CHIP("CPU", device->cpu);

	// oscillator
	device->oscillator = oscillator_create(10000, device->signal_pool, (OscillatorSignals) {
											.clk_out = SIGNAL(clock)
	});
	DEVICE_REGISTER_CHIP("OSC", device->oscillator);

	// power-on-reset
	DEVICE_REGISTER_CHIP("POR", poweronreset_create(1000000, device->signal_pool, (PowerOnResetSignals) {
											.reset_b = SIGNAL(reset_b)
	}));

	// ram
	device->ram = ram_8d16a_create(15, device->signal_pool, (Ram8d16aSignals) {
										.bus_address = signal_split(device->signals.bus_address, 0, 15),
										.bus_data = SIGNAL(bus_data),
										.ce_b = SIGNAL(a15)
	});
	DEVICE_REGISTER_CHIP("RAM", device->ram);

	// rom
	device->rom = rom_8d16a_create(14, device->signal_pool, (Rom8d16aSignals) {
										.bus_address = signal_split(device->signals.bus_address, 0, 14),
										.bus_data = SIGNAL(bus_data),
	});
	DEVICE_REGISTER_CHIP("ROM", device->rom);

	if (rom_data) {
		memcpy(device->rom->data_array, rom_data, arrlenu(rom_data));
	}

	// pia
	device->pia = chip_6520_create(device->signal_pool, (Chip6520Signals) {
										.bus_data = SIGNAL(bus_data),
										.enable = SIGNAL(clock),
										.reset_b = SIGNAL(reset_b),
										.rw = SIGNAL(cpu_rw),
										.cs0 = SIGNAL(a15),
										.cs1 = SIGNAL(high),
										.rs0 = signal_split(SIGNAL(bus_address), 0, 1),
										.rs1 = signal_split(SIGNAL(bus_address), 1, 1),
	});
	DEVICE_REGISTER_CHIP("PIA", device->pia);

	// lcd-module
	device->lcd = chip_hd44780_create(device->signal_pool, (ChipHd44780Signals) {
										.db4_7 = signal_split(device->pia->signals.port_a, 0, 4),
										.rs = signal_split(device->pia->signals.port_a, 7, 1),
										.rw = signal_split(device->pia->signals.port_a, 6, 1),
										.enable = signal_split(device->pia->signals.port_a, 5, 1)
	});
	DEVICE_REGISTER_CHIP("LCD", device->lcd);

	// keypad
	device->keypad = input_keypad_create(device->signal_pool, true, 4, 4, 500, 100, (InputKeypadSignals) {
										.rows = signal_split(device->pia->signals.port_b, 4, 4),
										.cols = signal_split(device->pia->signals.port_b, 0, 4)
	});
	DEVICE_REGISTER_CHIP("KEYPAD", device->keypad);
	signal_default_uint8(device->signal_pool, device->keypad->signals.cols, false);

	// custom chip for the glue logic
	DEVICE_REGISTER_CHIP("LOGIC", glue_logic_create(device));

	// copy some signals for easy access
	SIGNAL(ram_oe_b) = device->ram->signals.oe_b;
	SIGNAL(ram_we_b) = device->ram->signals.we_b;
	SIGNAL(rom_ce_b) = device->rom->signals.ce_b;
	SIGNAL(pia_cs2_b) = device->pia->signals.cs2_b;

	// register dependencies
	for (int32_t id = 0; id < arrlen(device->chips); ++id) {
		device->chips[id]->register_dependencies(device->chips[id]);
	}

	return device;
}

void dev_minimal_6502_destroy(DevMinimal6502 *device) {
	assert(device);

	cpu_6502_destroy(device->cpu);
	ram_8d16a_destroy(device->ram);
	rom_8d16a_destroy(device->rom);
	free(device);
}

void dev_minimal_6502_process(DevMinimal6502 *device) {
	assert(device);

	device_simulate_timestep((Device *) device);
}

void dev_minimal_6502_reset(DevMinimal6502 *device) {
	// FIXME: trigger the power-on-reset device again
	(void) device;
}

void dev_minimal_6502_rom_from_file(DevMinimal6502 *device, const char *filename) {
	file_load_binary_fixed(filename, device->rom->data_array, device->rom->data_size);
}

void dev_minimal_6502_ram_from_file(DevMinimal6502 *device, const char *filename) {
	file_load_binary_fixed(filename, device->ram->data_array, device->ram->data_size);
}
