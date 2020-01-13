// test/test_dev_minimal_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "dev_minimal_6502.h"
#include "cpu_6502_opcodes.h"

#include "stb/stb_ds.h"

static void *dev_minimal_6502_setup(const MunitParameter params[], void *user_data) {
	// build ROM image
	uint8_t *rom = NULL;
	
	// >> a simple program
	arrput(rom, OP_6502_LDA_IMM);
	arrput(rom, 10);
	arrput(rom, OP_6502_STA_ZP);
	arrput(rom, 0x61);
	arrput(rom, OP_6502_BRK);

	// >> make sure the buffer is the full 32k
	arrsetlen(rom, 1 << 15);

	// >> IRQ / NMI handlers
	rom[0xfe00 - 0x8000] = OP_6502_JMP_ABS;
	rom[0xfe01 - 0x8000] = 0x00;
	rom[0xfe02 - 0x8000] = 0xfe;

	// >> VECTORS
	rom[0xfffa - 0x8000] = 0x00;		// NMI vector - low
	rom[0xfffb - 0x8000] = 0xfe;		// NMI vector - high
	rom[0xfffc - 0x8000] = 0x00;		// reset vector - low
	rom[0xfffd - 0x8000] = 0x80;		// reset vector - high
	rom[0xfffe - 0x8000] = 0x00;		// IRQ vector - low
	rom[0xffff - 0x8000] = 0xfe;		// IRQ vector - high

	DevMinimal6502 *dev = dev_minimal_6502_create(rom);
	return dev;
}

static void dev_minimal_6502_teardown(void *fixture) {
	DevMinimal6502 *dev = (DevMinimal6502 *) fixture;
	dev_minimal_6502_destroy(dev);
}

MunitResult test_program(const MunitParameter params[], void *user_data_or_fixture) {

	DevMinimal6502 *dev = (DevMinimal6502 *) user_data_or_fixture;

	// initialize the memory that is used by the program
	dev->ram->data_array[0x61] = 0;

	// run the computer until the program counter reaches the IRQ-handler
	signal_write_bool(dev->signal_pool, dev->signals.reset_b, ACTLO_DEASSERT);

	int limit = 1000;

	while (limit > 0 && dev->cpu->reg_pc != 0xfe00) {
		dev_minimal_6502_process(dev);
		--limit;
	}

	// check some things
	munit_assert_int(limit, >, 0);
	munit_assert_uint8(dev->cpu->reg_a, ==, 10);
	munit_assert_uint8(dev->ram->data_array[0x61], ==, 10);

	return MUNIT_OK;
}

MunitTest dev_minimal_6502_tests[] = {
	{ "/run_program", test_program, dev_minimal_6502_setup, dev_minimal_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
