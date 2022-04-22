// dev_minimal_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a minimal MOS-6502 based system, with 32kb of RAM and a 16kb system ROM.

#include "dev_minimal_6502.h"

#include "crt.h"
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

///////////////////////////////////////////////////////////////////////////////
//
// internal - glue logic
//

#define SIGNAL_PREFIX		CHIP_GLUE_LOGIC_
#define SIGNAL_OWNER		chip

typedef enum {
	CHIP_GLUE_LOGIC_AB0 = CHIP_PIN_01,
	CHIP_GLUE_LOGIC_AB1,
	CHIP_GLUE_LOGIC_AB2,
	CHIP_GLUE_LOGIC_AB3,
	CHIP_GLUE_LOGIC_AB4,
	CHIP_GLUE_LOGIC_AB5,
	CHIP_GLUE_LOGIC_AB6,
	CHIP_GLUE_LOGIC_AB7,
	CHIP_GLUE_LOGIC_AB8,
	CHIP_GLUE_LOGIC_AB9,
	CHIP_GLUE_LOGIC_AB10,
	CHIP_GLUE_LOGIC_AB11,
	CHIP_GLUE_LOGIC_AB12,
	CHIP_GLUE_LOGIC_AB13,
	CHIP_GLUE_LOGIC_AB14,
	CHIP_GLUE_LOGIC_AB15,

	CHIP_GLUE_LOGIC_CPU_RW,
	CHIP_GLUE_LOGIC_CLOCK,
	CHIP_GLUE_LOGIC_RESET_BTN_B,

	CHIP_GLUE_LOGIC_RAM_OE_B,
	CHIP_GLUE_LOGIC_RAM_WE_B,
	CHIP_GLUE_LOGIC_ROM_CE_B,

	CHIP_GLUE_LOGIC_PIA_CS2_B,

	CHIP_GLUE_LOGIC_PIN_COUNT
} GlueLogicSignalAssignment;

typedef struct ChipGlueLogic {
	CHIP_DECLARE_BASE

	DevMinimal6502 *device;

	Signal			signals[CHIP_GLUE_LOGIC_PIN_COUNT];
	SignalPool *	signal_pool;

	SignalGroup		sg_address;
} ChipGlueLogic;

static uint8_t ChipGlueLogic_PinTypes[CHIP_GLUE_LOGIC_PIN_COUNT] = {0};

static void glue_logic_process(ChipGlueLogic *chip);
static void glue_logic_destroy(ChipGlueLogic *chip);

#define GLUE_PIN(s,t)					\
	chip->signals[pin] = s;				\
	chip->pin_types[pin] = (t);			\
	++pin;

static ChipGlueLogic *glue_logic_create(DevMinimal6502 *device) {
	ChipGlueLogic *chip = (ChipGlueLogic *) dms_calloc(1, sizeof(ChipGlueLogic));
	chip->device = device;
	chip->signal_pool = device->signal_pool;

	CHIP_SET_FUNCTIONS(chip, glue_logic_process, glue_logic_destroy);
	CHIP_SET_VARIABLES(chip, device->simulator, chip->signals, ChipGlueLogic_PinTypes, CHIP_GLUE_LOGIC_PIN_COUNT);

	int pin = 0;

	for (int i = 0; i < 16; ++i) {
		GLUE_PIN(*device->sg_address[i], CHIP_PIN_INPUT | CHIP_PIN_TRIGGER);
	}
	SIGNAL_GROUP(address) = signal_group_create_from_array(16, chip->signals);

	GLUE_PIN(device->signals[SIG_M6502_CPU_RW],      CHIP_PIN_INPUT | CHIP_PIN_TRIGGER);
	GLUE_PIN(device->signals[SIG_M6502_CLOCK],	     CHIP_PIN_INPUT | CHIP_PIN_TRIGGER);
	GLUE_PIN(device->signals[SIG_M6502_RESET_BTN_B], CHIP_PIN_OUTPUT);
	GLUE_PIN(device->signals[SIG_M6502_RAM_OE_B],    CHIP_PIN_OUTPUT);
	GLUE_PIN(device->signals[SIG_M6502_RAM_WE_B],    CHIP_PIN_OUTPUT);
	GLUE_PIN(device->signals[SIG_M6502_ROM_CE_B],    CHIP_PIN_OUTPUT);
	GLUE_PIN(device->signals[SIG_M6502_PIA_CS2_B],   CHIP_PIN_OUTPUT);

	assert(pin == CHIP_GLUE_LOGIC_PIN_COUNT);

	return chip;
}

static void glue_logic_destroy(ChipGlueLogic *chip) {
	assert(chip);
	signal_group_destroy(chip->sg_address);
	dms_free(chip);
}

static void glue_logic_process(ChipGlueLogic *chip) {
	assert(chip);
	DevMinimal6502 *device = chip->device;

	// >> reset logic
	SIGNAL_WRITE(RESET_BTN_B, !device->in_reset);
	device->in_reset = false;

	// >> ram logic
	//  - ce_b: assert when top bit of address isn't set (copy of a15)
	//	- oe_b: assert when cpu_rw is high
	//	- we_b: assert when cpu_rw is low and clock is high
	bool next_rw = SIGNAL_READ(CPU_RW);
	SIGNAL_WRITE(RAM_OE_B, !next_rw);
	SIGNAL_WRITE(RAM_WE_B, next_rw || !SIGNAL_READ(CLOCK));

	// >> rom logic
	//  - ce_b: assert when the top 2 bits of the address is set
	SIGNAL_WRITE(ROM_CE_B, !(SIGNAL_READ(AB15) && SIGNAL_READ(AB14)));

	// >> pia logic
	//  - no peripheral connected, irq lines not connected
	//	- cs0: assert when top bit of address is set (copy of a15)
	//	- cs1: always asserted
	//  - cs2_b: assert when bits 4-14 are zero
	uint16_t bus_address =  SIGNAL_GROUP_READ_U16(address);
	SIGNAL_WRITE(PIA_CS2_B, (bus_address & 0x7ff0) != 0x0000);
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

#undef  SIGNAL_PREFIX
#define SIGNAL_PREFIX		SIG_M6502_
#undef  SIGNAL_OWNER
#define SIGNAL_OWNER		device


Cpu6502* dev_minimal_6502_get_cpu(DevMinimal6502 *device) {
	assert(device);
	return device->cpu;
}

size_t dev_minimal_6502_get_irq_signals(DevMinimal6502 *device, SignalBreakpoint **irq_signals);

DevMinimal6502 *dev_minimal_6502_create(const uint8_t *rom_data) {
	DevMinimal6502 *device = (DevMinimal6502 *) dms_calloc(1, sizeof(DevMinimal6502));

	device->get_cpu = (DEVICE_GET_CPU) dev_minimal_6502_get_cpu;
	device->process = (DEVICE_PROCESS) device_process;
	device->reset = (DEVICE_RESET) dev_minimal_6502_reset;
	device->destroy = (DEVICE_DESTROY) dev_minimal_6502_destroy;
	device->read_memory = (DEVICE_READ_MEMORY) dev_minimal_6502_read_memory;
	device->write_memory = (DEVICE_WRITE_MEMORY) dev_minimal_6502_write_memory;
	device->get_irq_signals = (DEVICE_GET_IRQ_SIGNALS) dev_minimal_6502_get_irq_signals;

	device->simulator = simulator_create(NS_TO_PS(20));
	device->signal_pool = device->simulator->signal_pool;

	// signals
	SIGNAL_GROUP_NEW_N(address, 16, &SIGNAL(AB0), "cpu_address", "AB%d");

	SIGNAL_GROUP_NEW_N(data, 8, &SIGNAL(DB0), "cpu_data", "DB%d");

	SIGNAL_DEFINE_DEFAULT(CLOCK, true);
	SIGNAL_DEFINE_DEFAULT(RESET_BTN_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RESET_B, ACTLO_ASSERT);
	SIGNAL_DEFINE_DEFAULT(CPU_RW, true);
	SIGNAL_DEFINE_DEFAULT(CPU_IRQ_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(CPU_NMI_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE(CPU_SYNC);
	SIGNAL_DEFINE_DEFAULT(CPU_RDY, ACTHI_ASSERT);

	SIGNAL_DEFINE(RAM_OE_B);
	SIGNAL_DEFINE(RAM_WE_B);
	SIGNAL_DEFINE(ROM_CE_B);
	SIGNAL_DEFINE(PIA_CS2_B);

	SIGNAL_DEFINE_DEFAULT(LOW, false);
	SIGNAL_DEFINE_DEFAULT(HIGH, true);

	signal_set_name(SIGNAL_POOL, SIGNAL(CLOCK), "CLK");

	// cpu
	device->cpu = cpu_6502_create(device->simulator, (Cpu6502Signals) {
										[PIN_6502_AB0]  = SIGNAL(AB0),
										[PIN_6502_AB1]  = SIGNAL(AB1),
										[PIN_6502_AB2]  = SIGNAL(AB2),
										[PIN_6502_AB3]  = SIGNAL(AB3),
										[PIN_6502_AB4]  = SIGNAL(AB4),
										[PIN_6502_AB5]  = SIGNAL(AB5),
										[PIN_6502_AB6]  = SIGNAL(AB6),
										[PIN_6502_AB7]  = SIGNAL(AB7),
										[PIN_6502_AB8]  = SIGNAL(AB8),
										[PIN_6502_AB9]  = SIGNAL(AB9),
										[PIN_6502_AB10] = SIGNAL(AB10),
										[PIN_6502_AB11] = SIGNAL(AB11),
										[PIN_6502_AB12] = SIGNAL(AB12),
										[PIN_6502_AB13] = SIGNAL(AB13),
										[PIN_6502_AB14] = SIGNAL(AB14),
										[PIN_6502_AB15] = SIGNAL(AB15),

										[PIN_6502_DB0]  = SIGNAL(DB0),
										[PIN_6502_DB1]  = SIGNAL(DB1),
										[PIN_6502_DB2]  = SIGNAL(DB2),
										[PIN_6502_DB3]  = SIGNAL(DB3),
										[PIN_6502_DB4]  = SIGNAL(DB4),
										[PIN_6502_DB5]  = SIGNAL(DB5),
										[PIN_6502_DB6]  = SIGNAL(DB6),
										[PIN_6502_DB7]  = SIGNAL(DB7),

										[PIN_6502_CLK]   = SIGNAL(CLOCK),
										[PIN_6502_RES_B] = SIGNAL(RESET_B),
										[PIN_6502_RW]	 = SIGNAL(CPU_RW),
										[PIN_6502_IRQ_B] = SIGNAL(CPU_IRQ_B),
										[PIN_6502_NMI_B] = SIGNAL(CPU_NMI_B),
										[PIN_6502_SYNC]  = SIGNAL(CPU_SYNC),
										[PIN_6502_RDY]   = SIGNAL(CPU_RDY)
	});
	DEVICE_REGISTER_CHIP("CPU", device->cpu);

	// oscillator
	device->oscillator = oscillator_create(10000, device->simulator, (OscillatorSignals) {
										[CHIP_OSCILLATOR_CLK_OUT] = SIGNAL(CLOCK)
	});
	DEVICE_REGISTER_CHIP("OSC", device->oscillator);

	// power-on-reset
	DEVICE_REGISTER_CHIP("POR", poweronreset_create(1000000, device->simulator, (PowerOnResetSignals) {
										[CHIP_POR_TRIGGER_B] = SIGNAL(RESET_BTN_B),
										[CHIP_POR_RESET_B] = SIGNAL(RESET_B)
	}));

	// ram
	device->ram = ram_8d16a_create(15, device->simulator, (Ram8d16aSignals) {
										[CHIP_RAM8D16A_A0]  = SIGNAL(AB0),
										[CHIP_RAM8D16A_A1]  = SIGNAL(AB1),
										[CHIP_RAM8D16A_A2]  = SIGNAL(AB2),
										[CHIP_RAM8D16A_A3]  = SIGNAL(AB3),
										[CHIP_RAM8D16A_A4]  = SIGNAL(AB4),
										[CHIP_RAM8D16A_A5]  = SIGNAL(AB5),
										[CHIP_RAM8D16A_A6]  = SIGNAL(AB6),
										[CHIP_RAM8D16A_A7]  = SIGNAL(AB7),
										[CHIP_RAM8D16A_A8]  = SIGNAL(AB8),
										[CHIP_RAM8D16A_A9]  = SIGNAL(AB9),
										[CHIP_RAM8D16A_A10] = SIGNAL(AB10),
										[CHIP_RAM8D16A_A11] = SIGNAL(AB11),
										[CHIP_RAM8D16A_A12] = SIGNAL(AB12),
										[CHIP_RAM8D16A_A13] = SIGNAL(AB13),
										[CHIP_RAM8D16A_A14] = SIGNAL(AB14),
										[CHIP_RAM8D16A_A15] = SIGNAL(AB15),

										[CHIP_RAM8D16A_D0] = SIGNAL(DB0),
										[CHIP_RAM8D16A_D1] = SIGNAL(DB1),
										[CHIP_RAM8D16A_D2] = SIGNAL(DB2),
										[CHIP_RAM8D16A_D3] = SIGNAL(DB3),
										[CHIP_RAM8D16A_D4] = SIGNAL(DB4),
										[CHIP_RAM8D16A_D5] = SIGNAL(DB5),
										[CHIP_RAM8D16A_D6] = SIGNAL(DB6),
										[CHIP_RAM8D16A_D7] = SIGNAL(DB7),

										[CHIP_RAM8D16A_CE_B] = SIGNAL(AB15),
										[CHIP_RAM8D16A_OE_B] = SIGNAL(RAM_OE_B),
										[CHIP_RAM8D16A_WE_B] = SIGNAL(RAM_WE_B)
	});
	DEVICE_REGISTER_CHIP("RAM", device->ram);

	// rom
	device->rom = rom_8d16a_create(14, device->simulator, (Rom8d16aSignals) {
										[CHIP_ROM8D16A_A0]  = SIGNAL(AB0),
										[CHIP_ROM8D16A_A1]  = SIGNAL(AB1),
										[CHIP_ROM8D16A_A2]  = SIGNAL(AB2),
										[CHIP_ROM8D16A_A3]  = SIGNAL(AB3),
										[CHIP_ROM8D16A_A4]  = SIGNAL(AB4),
										[CHIP_ROM8D16A_A5]  = SIGNAL(AB5),
										[CHIP_ROM8D16A_A6]  = SIGNAL(AB6),
										[CHIP_ROM8D16A_A7]  = SIGNAL(AB7),
										[CHIP_ROM8D16A_A8]  = SIGNAL(AB8),
										[CHIP_ROM8D16A_A9]  = SIGNAL(AB9),
										[CHIP_ROM8D16A_A10] = SIGNAL(AB10),
										[CHIP_ROM8D16A_A11] = SIGNAL(AB11),
										[CHIP_ROM8D16A_A12] = SIGNAL(AB12),
										[CHIP_ROM8D16A_A13] = SIGNAL(AB13),

										[CHIP_ROM8D16A_D0] = SIGNAL(DB0),
										[CHIP_ROM8D16A_D1] = SIGNAL(DB1),
										[CHIP_ROM8D16A_D2] = SIGNAL(DB2),
										[CHIP_ROM8D16A_D3] = SIGNAL(DB3),
										[CHIP_ROM8D16A_D4] = SIGNAL(DB4),
										[CHIP_ROM8D16A_D5] = SIGNAL(DB5),
										[CHIP_ROM8D16A_D6] = SIGNAL(DB6),
										[CHIP_ROM8D16A_D7] = SIGNAL(DB7),

										[CHIP_ROM8D16A_CE_B] = SIGNAL(ROM_CE_B)
	});
	DEVICE_REGISTER_CHIP("ROM", device->rom);

	if (rom_data) {
		dms_memcpy(device->rom->data_array, rom_data, arrlenu(rom_data));
	}

	// pia
	device->pia = chip_6520_create(device->simulator, (Chip6520Signals) {
										[CHIP_6520_D0] = SIGNAL(DB0),
										[CHIP_6520_D1] = SIGNAL(DB1),
										[CHIP_6520_D2] = SIGNAL(DB2),
										[CHIP_6520_D3] = SIGNAL(DB3),
										[CHIP_6520_D4] = SIGNAL(DB4),
										[CHIP_6520_D5] = SIGNAL(DB5),
										[CHIP_6520_D6] = SIGNAL(DB6),
										[CHIP_6520_D7] = SIGNAL(DB7),
										[CHIP_6520_PHI2] = SIGNAL(CLOCK),
										[CHIP_6520_RESET_B] = SIGNAL(RESET_B),
										[CHIP_6520_RW] = SIGNAL(CPU_RW),
										[CHIP_6520_CS0] = SIGNAL(AB15),
										[CHIP_6520_CS1] = SIGNAL(HIGH),
										[CHIP_6520_RS0] = SIGNAL(AB0),
										[CHIP_6520_RS1] = SIGNAL(AB1),
										[CHIP_6520_CS2_B] = SIGNAL(PIA_CS2_B)
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
										 (Signal[]) {device->pia->signals[CHIP_6520_PB4], device->pia->signals[CHIP_6520_PB5],
													 device->pia->signals[CHIP_6520_PB6], device->pia->signals[CHIP_6520_PB7]},
										 (Signal[]) {device->pia->signals[CHIP_6520_PB0], device->pia->signals[CHIP_6520_PB1],
													 device->pia->signals[CHIP_6520_PB2], device->pia->signals[CHIP_6520_PB3]}
	);

	DEVICE_REGISTER_CHIP("KEYPAD", device->keypad);
	signal_group_defaults(SIGNAL_POOL, device->keypad->sg_rows, 0x00);

	// custom chip for the glue logic
	DEVICE_REGISTER_CHIP("LOGIC", glue_logic_create(device));


	// let the simulator know no more chips will be added
	simulator_device_complete(device->simulator);

	return device;
}

void dev_minimal_6502_destroy(DevMinimal6502 *device) {
	assert(device);

	signal_group_destroy(device->sg_address);
	signal_group_destroy(device->sg_data);

	simulator_destroy(device->simulator);
	dms_free(device);
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
		dms_memset(output, 0, size);
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

	for (int region = sr; remain > 0 && addr <= 0xffff && region < NUM_REGIONS; ++region) {
		size_t region_offset = addr - REGION_START[region];
		size_t to_copy = MIN(remain, REGION_SIZE[region] - region_offset);

		switch (region) {
			case 0:				// RAM
				dms_memcpy(output + done, device->ram->data_array + region_offset, to_copy);
				break;
			case 1:				// I/O area + unused
				dms_zero(output + done, to_copy);
				break;
			case 2:				// ROM
				dms_memcpy(output + done, device->rom->data_array + region_offset, to_copy);
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

	dms_memcpy(device->ram->data_array + start_address, input, real_size);
}

size_t dev_minimal_6502_get_irq_signals(DevMinimal6502 *device, SignalBreakpoint **irq_signals) {
	assert(device);
	assert(irq_signals);

	static SignalBreakpoint irqs = {
		.pos_edge = false,
		.neg_edge = false
	};

	if (signal_is_undefined(irqs.signal)) {
		irqs = (SignalBreakpoint) {SIGNAL(CPU_IRQ_B), false, true};
	}

	*irq_signals = &irqs;
	return 1;
}

void dev_minimal_6502_rom_from_file(DevMinimal6502 *device, const char *filename) {
	file_load_binary_fixed(filename, device->rom->data_array, device->rom->data_size);
}

void dev_minimal_6502_ram_from_file(DevMinimal6502 *device, const char *filename) {
	file_load_binary_fixed(filename, device->ram->data_array, device->ram->data_size);
}
