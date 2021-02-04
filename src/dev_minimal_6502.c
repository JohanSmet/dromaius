// dev_minimal_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a minimal MOS-6502 based system, with 32kb of RAM and a 16kb system ROM.

#include "dev_minimal_6502.h"

#include "utils.h"
#include "stb/stb_ds.h"

#include "chip_6520.h"
#include "chip_hd44780.h"
#include "chip_oscillator.h"
#include "chip_poweronreset.h"
#include "cpu_6502.h"
#include "input_keypad.h"
#include "ram_8d_16a.h"
#include "rom_8d_16a.h"

#include <assert.h>
#include <stdlib.h>

#define SIGNAL_POOL			device->simulator->signal_pool
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

	signal_add_dependency(SIGNAL_POOL, SIGNAL(cpu_rw), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(clock), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(bus_address), chip->id);
}

static void glue_logic_destroy(ChipGlueLogic *chip) {
	assert(chip);
	free(chip);
}

static void glue_logic_process(ChipGlueLogic *chip) {
	assert(chip);
	DevMinimal6502 *device = chip->device;

	// >> reset logic
	SIGNAL_SET_BOOL(reset_btn_b, !device->in_reset);
	device->in_reset = false;

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

size_t dev_minimal_6502_get_irq_signals(DevMinimal6502 *device, struct SignalBreak **irq_signals);

DevMinimal6502 *dev_minimal_6502_create(const uint8_t *rom_data) {
	DevMinimal6502 *device = (DevMinimal6502 *) calloc(1, sizeof(DevMinimal6502));

	device->get_cpu = (DEVICE_GET_CPU) dev_minimal_6502_get_cpu;
	device->process = (DEVICE_PROCESS) device_process;
	device->reset = (DEVICE_RESET) dev_minimal_6502_reset;
	device->destroy = (DEVICE_DESTROY) dev_minimal_6502_destroy;
	device->read_memory = (DEVICE_READ_MEMORY) dev_minimal_6502_read_memory;
	device->write_memory = (DEVICE_WRITE_MEMORY) dev_minimal_6502_write_memory;
	device->get_irq_signals = (DEVICE_GET_IRQ_SIGNALS) dev_minimal_6502_get_irq_signals;

	device->simulator = simulator_create(NS_TO_PS(20));

	// signals
	SIGNAL_DEFINE(bus_address, 16);
	SIGNAL_DEFINE(bus_data, 8);
	SIGNAL_DEFINE_BOOL_N(clock, 1, true, "CLK");
	SIGNAL_DEFINE_BOOL(reset_btn_b, 1, ACTLO_DEASSERT);
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
	device->cpu = cpu_6502_create(device->simulator, (Cpu6502Signals) {

										[PIN_6502_AB0]  = signal_split(SIGNAL(bus_address), 0, 1),
										[PIN_6502_AB1]  = signal_split(SIGNAL(bus_address), 1, 1),
										[PIN_6502_AB2]  = signal_split(SIGNAL(bus_address), 2, 1),
										[PIN_6502_AB3]  = signal_split(SIGNAL(bus_address), 3, 1),
										[PIN_6502_AB4]  = signal_split(SIGNAL(bus_address), 4, 1),
										[PIN_6502_AB5]  = signal_split(SIGNAL(bus_address), 5, 1),
										[PIN_6502_AB6]  = signal_split(SIGNAL(bus_address), 6, 1),
										[PIN_6502_AB7]  = signal_split(SIGNAL(bus_address), 7, 1),
										[PIN_6502_AB8]  = signal_split(SIGNAL(bus_address), 8, 1),
										[PIN_6502_AB9]  = signal_split(SIGNAL(bus_address), 9, 1),
										[PIN_6502_AB10] = signal_split(SIGNAL(bus_address), 10, 1),
										[PIN_6502_AB11] = signal_split(SIGNAL(bus_address), 11, 1),
										[PIN_6502_AB12] = signal_split(SIGNAL(bus_address), 12, 1),
										[PIN_6502_AB13] = signal_split(SIGNAL(bus_address), 13, 1),
										[PIN_6502_AB14] = signal_split(SIGNAL(bus_address), 14, 1),
										[PIN_6502_AB15] = signal_split(SIGNAL(bus_address), 15, 1),

										[PIN_6502_DB0]  = signal_split(SIGNAL(bus_data), 0, 1),
										[PIN_6502_DB1]  = signal_split(SIGNAL(bus_data), 1, 1),
										[PIN_6502_DB2]  = signal_split(SIGNAL(bus_data), 2, 1),
										[PIN_6502_DB3]  = signal_split(SIGNAL(bus_data), 3, 1),
										[PIN_6502_DB4]  = signal_split(SIGNAL(bus_data), 4, 1),
										[PIN_6502_DB5]  = signal_split(SIGNAL(bus_data), 5, 1),
										[PIN_6502_DB6]  = signal_split(SIGNAL(bus_data), 6, 1),
										[PIN_6502_DB7]  = signal_split(SIGNAL(bus_data), 7, 1),

										[PIN_6502_CLK]   = SIGNAL(clock),
										[PIN_6502_RES_B] = SIGNAL(reset_b),
										[PIN_6502_RW]	 = SIGNAL(cpu_rw),
										[PIN_6502_IRQ_B] = SIGNAL(cpu_irq_b),
										[PIN_6502_NMI_B] = SIGNAL(cpu_nmi_b),
										[PIN_6502_SYNC]  = SIGNAL(cpu_sync),
										[PIN_6502_RDY]   = SIGNAL(cpu_rdy)
	});
	DEVICE_REGISTER_CHIP("CPU", device->cpu);

	// oscillator
	device->oscillator = oscillator_create(10000, device->simulator, (OscillatorSignals) {
											[CHIP_OSCILLATOR_CLK_OUT] = SIGNAL(clock)
	});
	DEVICE_REGISTER_CHIP("OSC", device->oscillator);

	// power-on-reset
	DEVICE_REGISTER_CHIP("POR", poweronreset_create(1000000, device->simulator, (PowerOnResetSignals) {
											[CHIP_POR_TRIGGER_B] = SIGNAL(reset_btn_b),
											[CHIP_POR_RESET_B] = SIGNAL(reset_b)
	}));

	// ram
	device->ram = ram_8d16a_create(15, device->simulator, (Ram8d16aSignals) {
										[CHIP_RAM8D16A_A0] = signal_split(device->signals.bus_address, 0, 1),
										[CHIP_RAM8D16A_A1] = signal_split(device->signals.bus_address, 1, 1),
										[CHIP_RAM8D16A_A2] = signal_split(device->signals.bus_address, 2, 1),
										[CHIP_RAM8D16A_A3] = signal_split(device->signals.bus_address, 3, 1),
										[CHIP_RAM8D16A_A4] = signal_split(device->signals.bus_address, 4, 1),
										[CHIP_RAM8D16A_A5] = signal_split(device->signals.bus_address, 5, 1),
										[CHIP_RAM8D16A_A6] = signal_split(device->signals.bus_address, 6, 1),
										[CHIP_RAM8D16A_A7] = signal_split(device->signals.bus_address, 7, 1),
										[CHIP_RAM8D16A_A8] = signal_split(device->signals.bus_address, 8, 1),
										[CHIP_RAM8D16A_A9] = signal_split(device->signals.bus_address, 9, 1),
										[CHIP_RAM8D16A_A10] = signal_split(device->signals.bus_address, 10, 1),
										[CHIP_RAM8D16A_A11] = signal_split(device->signals.bus_address, 11, 1),
										[CHIP_RAM8D16A_A12] = signal_split(device->signals.bus_address, 12, 1),
										[CHIP_RAM8D16A_A13] = signal_split(device->signals.bus_address, 13, 1),
										[CHIP_RAM8D16A_A14] = signal_split(device->signals.bus_address, 14, 1),
										[CHIP_RAM8D16A_A15] = signal_split(device->signals.bus_address, 15, 1),

										[CHIP_RAM8D16A_D0] = signal_split(SIGNAL(bus_data), 0, 1),
										[CHIP_RAM8D16A_D1] = signal_split(SIGNAL(bus_data), 1, 1),
										[CHIP_RAM8D16A_D2] = signal_split(SIGNAL(bus_data), 2, 1),
										[CHIP_RAM8D16A_D3] = signal_split(SIGNAL(bus_data), 3, 1),
										[CHIP_RAM8D16A_D4] = signal_split(SIGNAL(bus_data), 4, 1),
										[CHIP_RAM8D16A_D5] = signal_split(SIGNAL(bus_data), 5, 1),
										[CHIP_RAM8D16A_D6] = signal_split(SIGNAL(bus_data), 6, 1),
										[CHIP_RAM8D16A_D7] = signal_split(SIGNAL(bus_data), 7, 1),

										[CHIP_RAM8D16A_CE_B] = SIGNAL(a15)
	});
	DEVICE_REGISTER_CHIP("RAM", device->ram);

	// rom
	device->rom = rom_8d16a_create(14, device->simulator, (Rom8d16aSignals) {
										[CHIP_RAM8D16A_A0] = signal_split(device->signals.bus_address, 0, 1),
										[CHIP_RAM8D16A_A1] = signal_split(device->signals.bus_address, 1, 1),
										[CHIP_RAM8D16A_A2] = signal_split(device->signals.bus_address, 2, 1),
										[CHIP_RAM8D16A_A3] = signal_split(device->signals.bus_address, 3, 1),
										[CHIP_RAM8D16A_A4] = signal_split(device->signals.bus_address, 4, 1),
										[CHIP_RAM8D16A_A5] = signal_split(device->signals.bus_address, 5, 1),
										[CHIP_RAM8D16A_A6] = signal_split(device->signals.bus_address, 6, 1),
										[CHIP_RAM8D16A_A7] = signal_split(device->signals.bus_address, 7, 1),
										[CHIP_RAM8D16A_A8] = signal_split(device->signals.bus_address, 8, 1),
										[CHIP_RAM8D16A_A9] = signal_split(device->signals.bus_address, 9, 1),
										[CHIP_RAM8D16A_A10] = signal_split(device->signals.bus_address, 10, 1),
										[CHIP_RAM8D16A_A11] = signal_split(device->signals.bus_address, 11, 1),
										[CHIP_RAM8D16A_A12] = signal_split(device->signals.bus_address, 12, 1),
										[CHIP_RAM8D16A_A13] = signal_split(device->signals.bus_address, 13, 1),

										[CHIP_RAM8D16A_D0] = signal_split(SIGNAL(bus_data), 0, 1),
										[CHIP_RAM8D16A_D1] = signal_split(SIGNAL(bus_data), 1, 1),
										[CHIP_RAM8D16A_D2] = signal_split(SIGNAL(bus_data), 2, 1),
										[CHIP_RAM8D16A_D3] = signal_split(SIGNAL(bus_data), 3, 1),
										[CHIP_RAM8D16A_D4] = signal_split(SIGNAL(bus_data), 4, 1),
										[CHIP_RAM8D16A_D5] = signal_split(SIGNAL(bus_data), 5, 1),
										[CHIP_RAM8D16A_D6] = signal_split(SIGNAL(bus_data), 6, 1),
										[CHIP_RAM8D16A_D7] = signal_split(SIGNAL(bus_data), 7, 1),
	});
	DEVICE_REGISTER_CHIP("ROM", device->rom);

	if (rom_data) {
		memcpy(device->rom->data_array, rom_data, arrlenu(rom_data));
	}

	// pia
	device->pia = chip_6520_create(device->simulator, (Chip6520Signals) {
										[CHIP_6520_D0] = signal_split(SIGNAL(bus_data), 0, 1),
										[CHIP_6520_D1] = signal_split(SIGNAL(bus_data), 1, 1),
										[CHIP_6520_D2] = signal_split(SIGNAL(bus_data), 2, 1),
										[CHIP_6520_D3] = signal_split(SIGNAL(bus_data), 3, 1),
										[CHIP_6520_D4] = signal_split(SIGNAL(bus_data), 4, 1),
										[CHIP_6520_D5] = signal_split(SIGNAL(bus_data), 5, 1),
										[CHIP_6520_D6] = signal_split(SIGNAL(bus_data), 6, 1),
										[CHIP_6520_D7] = signal_split(SIGNAL(bus_data), 7, 1),
										[CHIP_6520_PHI2] = SIGNAL(clock),
										[CHIP_6520_RESET_B] = SIGNAL(reset_b),
										[CHIP_6520_RW] = SIGNAL(cpu_rw),
										[CHIP_6520_CS0] = SIGNAL(a15),
										[CHIP_6520_CS1] = SIGNAL(high),
										[CHIP_6520_RS0] = signal_split(SIGNAL(bus_address), 0, 1),
										[CHIP_6520_RS1] = signal_split(SIGNAL(bus_address), 1, 1)
	});
	DEVICE_REGISTER_CHIP("PIA", device->pia);

	// lcd-module
	device->lcd = chip_hd44780_create(device->simulator, (ChipHd44780Signals) {
										[CHIP_HD44780_DB4] = device->pia->signals[CHIP_6520_PA0],
										[CHIP_HD44780_DB5] = device->pia->signals[CHIP_6520_PA1],
										[CHIP_HD44780_DB6] = device->pia->signals[CHIP_6520_PA2],
										[CHIP_HD44780_DB7] = device->pia->signals[CHIP_6520_PA3],
										[CHIP_HD44780_RS]  = device->pia->signals[CHIP_6520_PA7],
										[CHIP_HD44780_RW]  = device->pia->signals[CHIP_6520_PA6],
										[CHIP_HD44780_E]   = device->pia->signals[CHIP_6520_PA5]
	});
	DEVICE_REGISTER_CHIP("LCD", device->lcd);

	// keypad
	device->keypad = input_keypad_create(device->simulator, true, 4, 4, 500, 100,
										 (Signal[]) {device->pia->sg_port_b[4], device->pia->sg_port_b[5],
													 device->pia->sg_port_b[6], device->pia->sg_port_b[7]},
										 (Signal[]) {device->pia->sg_port_b[0], device->pia->sg_port_b[1],
													 device->pia->sg_port_b[2], device->pia->sg_port_b[3]}
	);

	DEVICE_REGISTER_CHIP("KEYPAD", device->keypad);
	signal_group_defaults(SIGNAL_POOL, device->keypad->sg_rows, 0x00);

	// custom chip for the glue logic
	DEVICE_REGISTER_CHIP("LOGIC", glue_logic_create(device));

	// copy some signals for easy access
	SIGNAL(ram_oe_b) = device->ram->signals[CHIP_RAM8D16A_OE_B];
	SIGNAL(ram_we_b) = device->ram->signals[CHIP_RAM8D16A_WE_B];
	SIGNAL(rom_ce_b) = device->rom->signals[CHIP_ROM8D16A_CE_B];
	SIGNAL(pia_cs2_b) = device->pia->signals[CHIP_6520_CS2_B];

	// let the simulator know no more chips will be added
	simulator_device_complete(device->simulator);

	return device;
}

void dev_minimal_6502_destroy(DevMinimal6502 *device) {
	assert(device);

	simulator_destroy(device->simulator);
	free(device);
}

void dev_minimal_6502_reset(DevMinimal6502 *device) {
	device->in_reset = true;
}

void dev_minimal_6502_read_memory(DevMinimal6502 *device, size_t start_address, size_t size, uint8_t *output) {
	assert(device);
	assert(output);

	static const size_t	REGION_START[] = {0x0000, 0x8000, 0xc000};
	static const size_t REGION_SIZE[]  = {0x8000, 0x4000, 0x4000};
	static const int NUM_REGIONS = sizeof(REGION_START) / sizeof(REGION_START[0]);

	if (start_address > 0xffff) {
		memset(output, 0, size);
		return;
	}

	// find start region
	int sr = NUM_REGIONS - 1;
	while (start_address < REGION_START[sr] && sr > 0) {
		sr -= 1;
	}

	size_t remain = size;
	size_t done = 0;
	size_t addr = start_address;

	for (int region = sr; remain > 0 && addr <= 0xffff; ++region) {
		size_t region_offset = addr - REGION_START[region];
		size_t to_copy = MIN(remain, REGION_SIZE[region] - region_offset);

		switch (region) {
			case 0:				// RAM
				memcpy(output + done, device->ram->data_array + region_offset, to_copy);
				break;
			case 1:				// I/O area + unused
				memset(output + done, 0, to_copy);
				break;
			case 2:				// ROM
				memcpy(output + done, device->rom->data_array + region_offset, to_copy);
				break;
		}

		remain -= to_copy;
		addr += to_copy;
		done += to_copy;
	}
}

void dev_minimal_6502_write_memory(DevMinimal6502 *device, size_t start_address, size_t size, uint8_t *input) {
	assert(device);
	assert(input);

	// only allow writes to the RAM area
	if (start_address > 0x7fff) {
		return;
	}

	size_t real_size = size;
	if (start_address + size > 0x8000) {
		real_size -= start_address + size - 0x8000;
	}

	memcpy(device->ram->data_array + start_address, input, real_size);
}

size_t dev_minimal_6502_get_irq_signals(DevMinimal6502 *device, struct SignalBreak **irq_signals) {
	assert(device);
	assert(irq_signals);

	static SignalBreak irqs[1] = {0};

	if (irqs[0].signal.count == 0) {
		irqs[0] = (SignalBreak) {device->signals.cpu_irq_b, false, true};
	}

	*irq_signals = irqs;
	return sizeof(irqs) / sizeof(irqs[0]);
}

void dev_minimal_6502_rom_from_file(DevMinimal6502 *device, const char *filename) {
	file_load_binary_fixed(filename, device->rom->data_array, device->rom->data_size);
}

void dev_minimal_6502_ram_from_file(DevMinimal6502 *device, const char *filename) {
	file_load_binary_fixed(filename, device->ram->data_array, device->ram->data_size);
}
