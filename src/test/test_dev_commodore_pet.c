// test/test_dev_commodore_pet.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "dev_commodore_pet.h"
#include "chip_ram_static.h"
#include "chip_ram_dynamic.h"
#include "chip_rom.h"

#include "cpu_6502.h"
#include "cpu_6502_opcodes.h"

#include <stdio.h>

#define SIGNAL_POOL			device->signal_pool
#define SIGNAL_COLLECTION	device->signals
#define SIGNAL_CHIP_ID		device->id

static void *dev_commodore_pet_setup(const MunitParameter params[], void *user_data) {
	DevCommodorePet *dev = dev_commodore_pet_create();
	return dev;
}

static void dev_commodore_pet_teardown(void *fixture) {
	DevCommodorePet *dev = (DevCommodorePet *) fixture;
	dev_commodore_pet_destroy(dev);
}

#define CPU_READ	true
#define CPU_WRITE	false

static uint16_t override_bus_address = 0;
static uint8_t  override_bus_data = 0;
static bool	    override_cpu_rw = CPU_READ;
static uint8_t  override_bus_bd = 0;

static void override_cpu_process(Cpu6502 *cpu) {
	signal_write_uint16(cpu->signal_pool, cpu->signals.bus_address, override_bus_address, cpu->id);
	signal_write_bool(cpu->signal_pool, cpu->signals.rw, override_cpu_rw, cpu->id);
	if (override_cpu_rw == CPU_WRITE) {
		signal_write_uint8(cpu->signal_pool, cpu->signals.bus_data, override_bus_data, cpu->id);
	}
}

static void override_ram_process(Chip8x4116DRam *ram) {
	signal_write_uint8(ram->signal_pool, ram->signals.bus_do, override_bus_bd, ram->id);
}

static void override_do_nothing(void *device) {
}

MunitResult test_signals_address(const MunitParameter params[], void *user_data_or_fixture) {

	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu and inject signals into the pet
	device->cpu->process = (CHIP_PROCESS_FUNC) override_cpu_process;

	// address area enable signals (selx_b)
	for (int addr = 0x0000; addr <= 0xffff; addr += 0x0100) {
		override_bus_address = addr & 0xffff;

		dev_commodore_pet_process_clk1(device);

		munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_ba), ==, override_bus_address);
		munit_assert(SIGNAL_NEXT_BOOL(sel4_b) == ((addr & 0xf000) != 0x4000));
		munit_assert(SIGNAL_NEXT_BOOL(sel5_b) == ((addr & 0xf000) != 0x5000));
		munit_assert(SIGNAL_NEXT_BOOL(sel6_b) == ((addr & 0xf000) != 0x6000));
		munit_assert(SIGNAL_NEXT_BOOL(sel7_b) == ((addr & 0xf000) != 0x7000));
		munit_assert(SIGNAL_NEXT_BOOL(sel8_b) == ((addr & 0xf000) != 0x8000));
		munit_assert(SIGNAL_NEXT_BOOL(sel9_b) == ((addr & 0xf000) != 0x9000));
		munit_assert(SIGNAL_NEXT_BOOL(sela_b) == ((addr & 0xf000) != 0xa000));
		munit_assert(SIGNAL_NEXT_BOOL(selb_b) == ((addr & 0xf000) != 0xb000));
		munit_assert(SIGNAL_NEXT_BOOL(selc_b) == ((addr & 0xf000) != 0xc000));
		munit_assert(SIGNAL_NEXT_BOOL(seld_b) == ((addr & 0xf000) != 0xd000));
		munit_assert(SIGNAL_NEXT_BOOL(sele_b) == ((addr & 0xf000) != 0xe000));
		munit_assert(SIGNAL_NEXT_BOOL(self_b) == ((addr & 0xf000) != 0xf000));
		munit_assert(SIGNAL_NEXT_BOOL(x8xx) == ((addr & 0x0f00) == 0x0800));
	}

	return MUNIT_OK;
}

MunitResult test_signals_data(const MunitParameter params[], void *user_data_or_fixture) {

	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu/ram and inject signals into the pet
	device->cpu->process = (CHIP_PROCESS_FUNC) override_cpu_process;
	device_chip_by_name((Device *) device, "I2-9")->process = (CHIP_PROCESS_FUNC) override_ram_process;
	device_chip_by_name((Device *) device, "J2-9")->process = (CHIP_PROCESS_FUNC) override_do_nothing;

	// data-bus read/write signals
	for (int addr = 0x0000; addr <= 0xffff; addr += 0x0100) {
		override_bus_address = addr & 0xffff;

		bool ro_addr = (addr >= 0x9000) || (addr & 0xff00) == 0x8800;
		bool rw_addr = !ro_addr;

		// reading: allow data to cpu when reading from a ram-address (ram_read_b low) or block when reading from other addres (ram_write_b low)
		override_cpu_rw = CPU_READ;
		dev_commodore_pet_process_clk1(device);
		munit_assert(!SIGNAL_NEXT_BOOL(ram_read_b) == rw_addr);
		munit_assert(!SIGNAL_NEXT_BOOL(ram_write_b) == ro_addr);

		// writing: always ok
		munit_logf(MUNIT_LOG_INFO, "%x", addr);
		override_cpu_rw = CPU_WRITE;
		dev_commodore_pet_process_clk1(device);
		munit_assert_true(SIGNAL_NEXT_BOOL(ram_read_b));
		munit_assert_false(SIGNAL_NEXT_BOOL(ram_write_b));
	}

	return MUNIT_OK;
}

MunitResult test_read_databus(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu/ram and inject signals into the pet
	device->cpu->process = (CHIP_PROCESS_FUNC) override_cpu_process;
	device_chip_by_name((Device *) device, "I2-9")->process = (CHIP_PROCESS_FUNC) override_ram_process;
	device_chip_by_name((Device *) device, "J2-9")->process = (CHIP_PROCESS_FUNC) override_do_nothing;
	device_chip_by_name((Device *)device, "F7")->process = (CHIP_PROCESS_FUNC) override_do_nothing;
	device_chip_by_name((Device *)device, "F8")->process = (CHIP_PROCESS_FUNC) override_do_nothing;

	for (int i = 0; device->roms[i] != NULL; ++i) {
		device->roms[i]->process = (CHIP_PROCESS_FUNC) override_do_nothing;
	}

	// reading from the databus
	for (int addr = 0x0000; addr < 0x8000; addr += 0x0100) {
		munit_logf(MUNIT_LOG_INFO, "%x", addr);
		override_bus_address = addr & 0xffff;
		override_cpu_rw = CPU_READ;
		override_bus_bd = (addr & 0xff00) >> 8;

		dev_commodore_pet_process_clk1(device);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, override_bus_bd);
	}

	return MUNIT_OK;
}

MunitResult test_write_databus(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu and inject signals into the pet
	device->cpu->process = (CHIP_PROCESS_FUNC) override_cpu_process;

	for (int i = 0; device->roms[i] != NULL; ++i) {
		device->roms[i]->process = (CHIP_PROCESS_FUNC) override_do_nothing;
	}

	// reading from the databus
	for (int addr = 0x0000; addr < 0x8800; addr += 0x0100) {
		override_bus_address = addr & 0xffff;
		override_bus_data = (addr & 0xff00) >> 8;
		override_cpu_rw = CPU_WRITE;

		dev_commodore_pet_process_clk1(device);

		// simulate a few more cycles to let everything settle
		device->process(device);
		device->process(device);
		device->process(device);
		device->process(device);

		bool ro_addr = (addr >= 0x9000) || (addr & 0xff00) == 0x8800;
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_bd), ==, (!ro_addr) ? override_bus_data : 0);
	}

	return MUNIT_OK;
}

MunitResult test_ram(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu and inject signals into the pet
	device->cpu->process = (CHIP_PROCESS_FUNC) override_cpu_process;

	if (SIGNAL_BOOL(clk1)) {
		dev_commodore_pet_process_clk1(device);
	}

	Chip8x4116DRam *ram_chips[] = {
		(Chip8x4116DRam *) device_chip_by_name((Device *) device, "I2-9"),
		(Chip8x4116DRam *) device_chip_by_name((Device *) device, "J2-9")
	};

	for (int addr = 0x0000; addr <= 0x7fff; ++addr) {
		override_bus_address = addr & 0xffff;
		override_bus_data = addr & 0xff;
		override_cpu_rw = CPU_WRITE;

		dev_commodore_pet_process_clk1(device);
		munit_assert_true(SIGNAL_BOOL(clk1));

		dev_commodore_pet_process_clk1(device);
		munit_assert_false(SIGNAL_BOOL(clk1));
	}

	for (int addr = 0x0000; addr <= 0x7fff; ++addr) {
		override_bus_address = addr & 0xffff;
		override_cpu_rw = CPU_READ;

		dev_commodore_pet_process_clk1(device);
		munit_assert_true(SIGNAL_BOOL(clk1));

		dev_commodore_pet_process_clk1(device);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, addr & 0xff);
		munit_assert_false(SIGNAL_BOOL(clk1));
	}

	return MUNIT_OK;
}

MunitResult test_vram(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu and inject signals into the pet
	device->cpu->process = (CHIP_PROCESS_FUNC) override_cpu_process;

	if (SIGNAL_BOOL(clk1)) {
		dev_commodore_pet_process_clk1(device);
	}

	Chip6114SRam *ram_hi = (Chip6114SRam *) device_chip_by_name((Device *)device, "F7");
	Chip6114SRam *ram_lo = (Chip6114SRam *) device_chip_by_name((Device *)device, "F8");

	for (int addr = 0x8000; addr <= 0x83e8; ++addr) {		// 40x25
		override_bus_address = addr & 0xffff;
		override_bus_data = addr & 0xff;
		override_cpu_rw = CPU_WRITE;

		dev_commodore_pet_process_clk1(device);
		munit_assert_true(SIGNAL_BOOL(clk1));

		dev_commodore_pet_process_clk1(device);
		munit_assert_uint8(ram_lo->data_array[override_bus_address - 0x8000], ==, override_bus_data & 0x0f);
		munit_assert_uint8(ram_hi->data_array[override_bus_address - 0x8000], ==, (override_bus_data & 0xf0) >> 4);
		munit_assert_false(SIGNAL_BOOL(clk1));
	}

	for (int addr = 0x8000; addr <= 0x83e8; ++addr) {		// 40x25
		override_bus_address = addr & 0xffff;
		override_cpu_rw = CPU_READ;

		dev_commodore_pet_process_clk1(device);
		munit_assert_true(SIGNAL_BOOL(clk1));

		dev_commodore_pet_process_clk1(device);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, addr & 0xff);
		munit_assert_false(SIGNAL_BOOL(clk1));
	}

	return MUNIT_OK;
}

MunitResult test_rom(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu and inject signals into the pet
	device->cpu->process = (CHIP_PROCESS_FUNC) override_cpu_process;

	if (SIGNAL_BOOL(clk1)) {
		dev_commodore_pet_process_clk1(device);
	}

	static int rom_addr[] = {0xb000, 0xc000, 0xd000, 0xe000, 0xf000};

	for (int rom = 0; device->roms[rom] != NULL; ++rom) {
		for (int offset = 0; offset < device->roms[rom]->data_size; ++offset) {
			override_bus_address = (rom_addr[rom] + offset) & 0xffff;
			override_cpu_rw = CPU_READ;

			dev_commodore_pet_process_clk1(device);
			munit_assert_true(SIGNAL_BOOL(clk1));

			dev_commodore_pet_process_clk1(device);
			device->process(device);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, device->roms[rom]->data_array[offset]);
			munit_assert_false(SIGNAL_BOOL(clk1));
		}
	}

	return MUNIT_OK;
}

MunitResult test_startup(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	for (int cycle = 0; cycle < 10000; ++cycle) {
		device->process(device);
	}

	return MUNIT_OK;
}

MunitResult test_vram_program(const MunitParameter params[], void *user_data_or_fixture) {
// test display ram with all the components in the system actually running

	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// replace the kernal rom with a custom ROM
	uint8_t *rom = device->roms[4]->data_array;
	int idx = 0;

	// >> init stack pointer
	rom[idx++] = OP_6502_LDX_IMM;	// ldx #$ff
	rom[idx++] = 0xff;
	rom[idx++] = OP_6502_TXS;		// txs
	rom[idx++] = OP_6502_INX;		// inx	- reg_x == 0

	// >> disable interrupts
	rom[idx++] = OP_6502_SEI;		// sei

	// >> write the first 1024 bytes of the vram
	rom[idx++] = OP_6502_LDA_IMM;	// lda #$5a
	rom[idx++] = 0x5a;
	rom[idx++] = OP_6502_STA_ABSX;	// sta $8000,x
	rom[idx++] = 0x00;
	rom[idx++] = 0x80;
	rom[idx++] = OP_6502_STA_ABSX;	// sta $8100,x
	rom[idx++] = 0x00;
	rom[idx++] = 0x81;
	rom[idx++] = OP_6502_STA_ABSX;	// sta $8200,x
	rom[idx++] = 0x00;
	rom[idx++] = 0x82;
	rom[idx++] = OP_6502_STA_ABSX;	// sta $8300,x
	rom[idx++] = 0x00;
	rom[idx++] = 0x83;
	rom[idx++] = OP_6502_DEX;		// dex
	rom[idx++] = OP_6502_BNE;		// bne $f1
	rom[idx++] = 0xf1;

	// >> end program
	rom[idx++] = OP_6502_BRK;

	// >> IRQ / NMI handlers
	rom[0xfe00 - 0xf000] = OP_6502_JMP_ABS;
	rom[0xfe01 - 0xf000] = 0x00;
	rom[0xfe02 - 0xf000] = 0xfe;

	// >> VECTORS
	rom[0xfffa - 0xf000] = 0x00;		// NMI vector - low
	rom[0xfffb - 0xf000] = 0xfe;		// NMI vector - high
	rom[0xfffc - 0xf000] = 0x00;		// reset vector - low
	rom[0xfffd - 0xf000] = 0xf0;		// reset vector - high
	rom[0xfffe - 0xf000] = 0x00;		// IRQ vector - low
	rom[0xffff - 0xf000] = 0xfe;		// IRQ vector - high

	// run the program until the irq handler is executed
	while (device->cpu->reg_pc != 0xfe00) {
		dev_commodore_pet_process_clk1(device);
	}

	// check ram
	Chip6114SRam *ram_hi = (Chip6114SRam *) device_chip_by_name((Device *)device, "F7");
	Chip6114SRam *ram_lo = (Chip6114SRam *) device_chip_by_name((Device *)device, "F8");


	for (int i = 0; i < 1024; ++i) {
		munit_assert_uint8(ram_hi->data_array[i], ==, 0x05);
		munit_assert_uint8(ram_lo->data_array[i], ==, 0x0a);
	}

	return MUNIT_OK;
}

static MunitResult test_read_write_memory(const MunitParameter params[], void *user_data_or_fixture) {

	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	uint8_t src_buffer[16] = {0x01, 0x20, 0x03, 0x40, 0x05, 0x60, 0x07, 0x80, 0x09, 0xa0, 0x0b, 0xc0, 0x0d, 0xe0, 0x0f, 0x15};
	uint8_t dst_buffer[16];

	// test main memory
	device->write_memory(device, 0x3ff8, sizeof(src_buffer), src_buffer);

	memset(dst_buffer, 0xaa, sizeof(dst_buffer));
	device->read_memory(device, 0x3ff8, sizeof(dst_buffer), dst_buffer);

	munit_assert_memory_equal(sizeof(src_buffer), src_buffer, dst_buffer);

	// test display memory
	device->write_memory(device, 0x8008, sizeof(src_buffer), src_buffer);

	memset(dst_buffer, 0xaa, sizeof(dst_buffer));
	device->read_memory(device, 0x8008, sizeof(dst_buffer), dst_buffer);

	munit_assert_memory_equal(sizeof(src_buffer), src_buffer, dst_buffer);

	return MUNIT_OK;
}

MunitTest dev_commodore_pet_tests[] = {
	{ "/address_signals", test_signals_address, dev_commodore_pet_setup, dev_commodore_pet_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/address_data", test_signals_data, dev_commodore_pet_setup, dev_commodore_pet_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_databus", test_read_databus, dev_commodore_pet_setup, dev_commodore_pet_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_databus", test_write_databus, dev_commodore_pet_setup, dev_commodore_pet_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ram", test_ram, dev_commodore_pet_setup, dev_commodore_pet_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/vram", test_vram, dev_commodore_pet_setup, dev_commodore_pet_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/rom", test_rom, dev_commodore_pet_setup, dev_commodore_pet_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/startup", test_startup, dev_commodore_pet_setup, dev_commodore_pet_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/vram_program", test_vram_program, dev_commodore_pet_setup, dev_commodore_pet_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_write_memory", test_read_write_memory, dev_commodore_pet_setup, dev_commodore_pet_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
