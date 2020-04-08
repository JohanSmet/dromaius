// test/test_dev_commodore_pet.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "dev_commodore_pet.h"
#include "cpu_6502_opcodes.h"
#include <stdio.h>

#define SIGNAL_POOL			device->signal_pool
#define SIGNAL_COLLECTION	device->signals

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

static void override_cpu_process(Cpu6502 *cpu, bool delayed) {
	signal_write_uint16(cpu->signal_pool, cpu->signals.bus_address, override_bus_address);
	signal_write_bool(cpu->signal_pool, cpu->signals.rw, override_cpu_rw);
	if (override_cpu_rw == CPU_WRITE) {
		signal_write_uint8(cpu->signal_pool, cpu->signals.bus_data, override_bus_data);
	}
}

static void override_ram_process(Ram8d16a *ram) {
	signal_write_uint8(ram->signal_pool, ram->signals.bus_data, override_bus_bd);
}

static void override_do_nothing(void *device) {
}

MunitResult test_signals_address(const MunitParameter params[], void *user_data_or_fixture) {

	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu and inject signals into the pet
	device->cpu->process = (CPU_PROCESS_FUNC) override_cpu_process;

	// address area enable signals (selx_b)
	for (int addr = 0x0000; addr <= 0xffff; addr += 0x0100) {
		override_bus_address = addr & 0xffff;

		dev_commodore_pet_process(device);

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
	device->cpu->process = (CPU_PROCESS_FUNC) override_cpu_process;
	device->ram->process = (PROCESS_FUNC) override_ram_process;

	// data-bus read/write signals
	for (int addr = 0x0000; addr <= 0xffff; addr += 0x0100) {
		override_bus_address = addr & 0xffff;

		bool ro_addr = (addr >= 0x9000) || (addr & 0xff00) == 0x8800;
		bool rw_addr = !ro_addr;

		// reading: allow data to cpu when reading from a ram-address (ram_read_b low) or block when reading from other addres (ram_write_b low)
		override_cpu_rw = CPU_READ;
		dev_commodore_pet_process(device);
		munit_assert(!SIGNAL_NEXT_BOOL(ram_read_b) == rw_addr);
		munit_assert(!SIGNAL_NEXT_BOOL(ram_write_b) == ro_addr);

		// writing: always ok
		munit_logf(MUNIT_LOG_INFO, "%x", addr);
		override_cpu_rw = CPU_WRITE;
		dev_commodore_pet_process(device);
		munit_assert_true(SIGNAL_NEXT_BOOL(ram_read_b));
		munit_assert_false(SIGNAL_NEXT_BOOL(ram_write_b));
	}

	return MUNIT_OK;
}

MunitResult test_read_databus(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu/ram and inject signals into the pet
	device->cpu->process = (CPU_PROCESS_FUNC) override_cpu_process;
	device->ram->process = (PROCESS_FUNC) override_ram_process;
	device->vram->process = (PROCESS_FUNC) override_do_nothing;

	for (int i = 0; device->roms[i] != NULL; ++i) {
		device->roms[i]->process = (PROCESS_FUNC) override_do_nothing;
	}

	// reading from the databus
	for (int addr = 0x0000; addr < 0x8800; addr += 0x0100) {
		override_bus_address = addr & 0xffff;
		override_cpu_rw = CPU_READ;
		override_bus_bd = (addr & 0xff00) >> 8;

		dev_commodore_pet_process(device);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, override_bus_bd);
	}

	return MUNIT_OK;
}

MunitResult test_write_databus(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu and inject signals into the pet
	device->cpu->process = (CPU_PROCESS_FUNC) override_cpu_process;

	for (int i = 0; device->roms[i] != NULL; ++i) {
		device->roms[i]->process = (PROCESS_FUNC) override_do_nothing;
	}

	// reading from the databus
	for (int addr = 0x0000; addr < 0x8800; addr += 0x0100) {
		override_bus_address = addr & 0xffff;
		override_bus_data = (addr & 0xff00) >> 8;
		override_cpu_rw = CPU_WRITE;

		dev_commodore_pet_process(device);

		bool ro_addr = (addr >= 0x9000) || (addr & 0xff00) == 0x8800;
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_bd), ==, (!ro_addr) ? override_bus_data : 0);
	}

	return MUNIT_OK;
}

MunitResult test_ram(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu and inject signals into the pet
	device->cpu->process = (CPU_PROCESS_FUNC) override_cpu_process;

	for (int addr = 0x0000; addr <= 0x7fff; ++addr) {
		override_bus_address = addr & 0xffff;
		override_bus_data = addr & 0xff;
		override_cpu_rw = CPU_WRITE;

		dev_commodore_pet_process(device);
		munit_assert_true(SIGNAL_NEXT_BOOL(clk1));

		dev_commodore_pet_process(device);
		munit_assert_uint8(device->ram->data_array[override_bus_address], ==, override_bus_data);
		munit_assert_false(SIGNAL_NEXT_BOOL(clk1));
	}

	for (int addr = 0x0000; addr <= 0x7fff; ++addr) {
		override_bus_address = addr & 0xffff;
		override_cpu_rw = CPU_READ;

		dev_commodore_pet_process(device);
		munit_assert_true(SIGNAL_NEXT_BOOL(clk1));
		munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, addr & 0xff);

		dev_commodore_pet_process(device);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, addr & 0xff);
		munit_assert_false(SIGNAL_NEXT_BOOL(clk1));
	}

	return MUNIT_OK;
}

MunitResult test_vram(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu and inject signals into the pet
	device->cpu->process = (CPU_PROCESS_FUNC) override_cpu_process;

	for (int addr = 0x8000; addr <= 0x83e8; ++addr) {		// 40x25
		override_bus_address = addr & 0xffff;
		override_bus_data = addr & 0xff;
		override_cpu_rw = CPU_WRITE;

		dev_commodore_pet_process(device);
		munit_assert_true(SIGNAL_NEXT_BOOL(clk1));

		dev_commodore_pet_process(device);
		munit_assert_uint8(device->vram->data_array[override_bus_address - 0x8000], ==, override_bus_data);
		munit_assert_false(SIGNAL_NEXT_BOOL(clk1));
	}

	for (int addr = 0x8000; addr <= 0x83e8; ++addr) {		// 40x25
		override_bus_address = addr & 0xffff;
		override_cpu_rw = CPU_READ;

		dev_commodore_pet_process(device);
		munit_assert_true(SIGNAL_NEXT_BOOL(clk1));
		munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, addr & 0xff);

		dev_commodore_pet_process(device);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, addr & 0xff);
		munit_assert_false(SIGNAL_NEXT_BOOL(clk1));
	}

	return MUNIT_OK;
}

MunitResult test_rom(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	// 'remove' cpu and inject signals into the pet
	device->cpu->process = (CPU_PROCESS_FUNC) override_cpu_process;

	static int rom_addr[] = {0xb000, 0xc000, 0xd000, 0xe000, 0xf000};

	for (int rom = 0; device->roms[rom] != NULL; ++rom) {
		for (int offset = 0; offset < device->roms[rom]->data_size; ++offset) {
			override_bus_address = (rom_addr[rom] + offset) & 0xffff;
			override_cpu_rw = CPU_READ;

			dev_commodore_pet_process(device);
			munit_assert_true(SIGNAL_NEXT_BOOL(clk1));
			//munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, device->roms[rom]->data_array[offset]);

			dev_commodore_pet_process(device);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(cpu_bus_data), ==, device->roms[rom]->data_array[offset]);
			munit_assert_false(SIGNAL_NEXT_BOOL(clk1));
		}
	}

	return MUNIT_OK;
}

MunitResult test_startup(const MunitParameter params[], void *user_data_or_fixture) {
	DevCommodorePet *device = (DevCommodorePet *) user_data_or_fixture;

	for (int cycle = 0; cycle < 10000; ++cycle) {
		dev_commodore_pet_process(device);
	}

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
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
