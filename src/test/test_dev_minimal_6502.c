// test/test_dev_minimal_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
#include "munit/munit.h"
#include "dev_minimal_6502.h"

#include "crt.h"
#include "chip_6520.h"
#include "cpu_6502.h"
#include "cpu_6502_opcodes.h"
#include "ram_8d_16a.h"

#include "stb/stb_ds.h"

static DevMinimal6502 *dev_minimal_6502_setup(int program) {

	// build ROM image
	uint8_t *rom = NULL;

	// >> a simple program
	if (program == 0) {
		arrput(rom, OP_6502_INC_ZP);
		arrput(rom, 0x00);
		arrput(rom, OP_6502_LDA_IMM);
		arrput(rom, 10);
		arrput(rom, OP_6502_STA_ZP);
		arrput(rom, 0x61);
		arrput(rom, OP_6502_BRK);
	} else if (program == 1) {
		arrput(rom, OP_6502_LDA_IMM);
		arrput(rom, 0xff);
		arrput(rom, OP_6502_STA_ABS);		// write the PIA - DDRA
		arrput(rom, 0x00);
		arrput(rom, 0x80);
		arrput(rom, OP_6502_LDA_IMM);
		arrput(rom, 0xf0);
		arrput(rom, OP_6502_STA_ABS);		// write the PIA - DDRB
		arrput(rom, 0x02);
		arrput(rom, 0x80);
		arrput(rom, OP_6502_LDA_IMM);
		arrput(rom, 0b00000100);
		arrput(rom, OP_6502_STA_ABS);		// write the PIA - CRA (select ORA instead of DDRA)
		arrput(rom, 0x01);
		arrput(rom, 0x80);
		arrput(rom, OP_6502_STA_ABS);		// write the PIA - CRB (select ORA instead of DDRB)
		arrput(rom, 0x03);
		arrput(rom, 0x80);
		arrput(rom, OP_6502_LDA_IMM);
		arrput(rom, 0x55);
		arrput(rom, OP_6502_STA_ABS);		// write the PIA - ORA
		arrput(rom, 0x00);
		arrput(rom, 0x80);
		arrput(rom, OP_6502_STA_ABS);		// write the PIA - ORB
		arrput(rom, 0x02);
		arrput(rom, 0x80);
		arrput(rom, OP_6502_BRK);
	} else {
		arrput(rom, OP_6502_BRK);
	}

	// >> make sure the buffer is the full 16k
	arrsetlen(rom, 1 << 14);

	// >> IRQ / NMI handlers
	rom[0xfe00 - 0xC000] = OP_6502_JMP_ABS;
	rom[0xfe01 - 0xC000] = 0x00;
	rom[0xfe02 - 0xC000] = 0xfe;

	// >> VECTORS
	rom[0xfffa - 0xC000] = 0x00;		// NMI vector - low
	rom[0xfffb - 0xC000] = 0xfe;		// NMI vector - high
	rom[0xfffc - 0xC000] = 0x00;		// reset vector - low
	rom[0xfffd - 0xC000] = 0xc0;		// reset vector - high
	rom[0xfffe - 0xC000] = 0x00;		// IRQ vector - low
	rom[0xffff - 0xC000] = 0xfe;		// IRQ vector - high

	DevMinimal6502 *dev = dev_minimal_6502_create(rom);
	return dev;
}

static void dev_minimal_6502_teardown(void *fixture) {
	DevMinimal6502 *dev = (DevMinimal6502 *) fixture;
	dev_minimal_6502_destroy(dev);
}

MunitResult test_program(const MunitParameter params[], void *user_data_or_fixture) {

	DevMinimal6502 *dev = dev_minimal_6502_setup(0);

	// initialize the memory that is used by the program
	dev->ram->data_array[0x00] = 0;
	dev->ram->data_array[0x61] = 0;

	// run the computer until the program counter reaches the IRQ-handler
	int limit = 1000;

	while (limit > 0 && dev->cpu->reg_pc != 0xfe00) {
		dev->process(dev);
		--limit;
	}

	// check some things
	munit_assert_int(limit, >, 0);
	munit_assert_uint8(dev->cpu->reg_a, ==, 10);
	munit_assert_uint8(dev->ram->data_array[0x00], ==, 1);
	munit_assert_uint8(dev->ram->data_array[0x61], ==, 10);

	dev_minimal_6502_teardown(dev);

	return MUNIT_OK;
}

MunitResult test_pia(const MunitParameter params[], void *user_data_or_fixture) {

	DevMinimal6502 *dev = dev_minimal_6502_setup(1);

	// run the computer until the program counter reaches the IRQ-handler
	int limit = 1000;

	while (limit > 0 && dev->cpu->reg_pc != 0xfe00) {
		dev->process(dev);
		--limit;
	}

	// check some things - not an exhaustive test
	munit_assert_int(limit, >, 0);
	munit_assert_uint8(dev->pia->reg_ddra, ==, 0xff);
	munit_assert_uint8(dev->pia->reg_ddrb, ==, 0xf0);
	munit_assert_uint8(dev->pia->reg_cra, ==, 0x04);
	munit_assert_uint8(dev->pia->reg_crb, ==, 0x04);
	munit_assert_uint8(dev->pia->reg_ora, ==, 0x55);
	munit_assert_uint8(dev->pia->reg_orb, ==, 0x55);
	munit_assert_uint8(signal_group_read(dev->simulator->signal_pool, dev->pia->sg_port_a), ==, 0x55);
	munit_assert_uint8(signal_group_read(dev->simulator->signal_pool, dev->pia->sg_port_b), ==, 0x50);	// lower nibble == keypad output

	dev_minimal_6502_teardown(dev);

	return MUNIT_OK;
}

static MunitResult test_read_write_memory(const MunitParameter params[], void *user_data_or_fixture) {

	DevMinimal6502 *dev = dev_minimal_6502_setup(0);

	uint8_t src_buffer[16] = {0x01, 0x20, 0x03, 0x40, 0x05, 0x60, 0x07, 0x80, 0x09, 0xa0, 0x0b, 0xc0, 0x0d, 0xe0, 0x0f, 0x15};
	uint8_t dst_buffer[16];

	// test main memory
	dev->write_memory(dev, 0x1000, sizeof(src_buffer), src_buffer);

	dms_memset(dst_buffer, 0xaa, sizeof(dst_buffer));
	dev->read_memory(dev, 0x1000, sizeof(dst_buffer), dst_buffer);

	munit_assert_memory_equal(sizeof(src_buffer), src_buffer, dst_buffer);

	dev_minimal_6502_teardown(dev);

	return MUNIT_OK;
}

MunitTest dev_minimal_6502_tests[] = {
	{ "/run_program", test_program, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/pia", test_pia, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_write_memory", test_read_write_memory, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
