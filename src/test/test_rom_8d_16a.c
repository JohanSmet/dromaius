// test/test_rom_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "rom_8d_16a.h"
#include "simulator.h"

#define SIGNAL_PREFIX		CHIP_ROM8D16A_
#define SIGNAL_OWNER		rom

static void *rom_8d16a_setup(const MunitParameter params[], void *user_data) {
	Rom8d16a *rom = rom_8d16a_create(16, simulator_create(NS_TO_PS(10)), (Rom8d16aSignals) {0});

	for (uint32_t i = 0; i <= 0xffff; ++i) {
		rom->data_array[i] = i & 0xff;
	}
	return rom;
}

static void rom_8d16a_teardown(void *fixture) {
	Rom8d16a *rom = (Rom8d16a *) fixture;
	simulator_destroy(rom->simulator);
	rom->destroy(rom);
}

static MunitResult test_read(const MunitParameter params[], void *user_data_or_fixture) {

	Rom8d16a *rom = (Rom8d16a *) user_data_or_fixture;

	SIGNAL_WRITE(CE_B, ACTLO_DEASSERT);
	signal_pool_cycle(rom->signal_pool);

	for (uint32_t i = 0; i <= 0xffff; ++i) {
		SIGNAL_WRITE(CE_B, ACTLO_ASSERT);
		SIGNAL_GROUP_WRITE(address, i);
		signal_pool_cycle(rom->signal_pool);
		rom->simulator->current_tick += 1;

		rom->process(rom);
		munit_assert_int64(rom->schedule_timestamp, !=, 0);

		signal_pool_cycle(rom->signal_pool);
		rom->simulator->current_tick += 1;
		rom->schedule_timestamp = 0;

		rom->process(rom);
		munit_assert_int64(rom->schedule_timestamp, ==, 0);
		munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, i & 0xff);
	}

	return MUNIT_OK;
}

static MunitResult test_ce(const MunitParameter params[], void *user_data_or_fixture) {

	Rom8d16a *rom = (Rom8d16a *) user_data_or_fixture;

	SIGNAL_WRITE(CE_B, ACTLO_DEASSERT);
	SIGNAL_GROUP_WRITE(address, 0x1635);
	signal_pool_cycle(rom->signal_pool);
	rom->simulator->current_tick += 1;
	rom->process(rom);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0);

	signal_pool_cycle(rom->signal_pool);
	rom->simulator->current_tick += 1;
	rom->process(rom);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0);

	SIGNAL_WRITE(CE_B, ACTLO_ASSERT);
	SIGNAL_GROUP_WRITE(address, 0x1635);
	signal_pool_cycle(rom->signal_pool);
	rom->simulator->current_tick += 1;
	rom->process(rom);
	signal_pool_cycle(rom->signal_pool);
	rom->simulator->current_tick += 1;
	rom->process(rom);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0x35);

	SIGNAL_WRITE(CE_B, ACTLO_DEASSERT);
	SIGNAL_GROUP_WRITE(address, 0x12AF);
	signal_pool_cycle(rom->signal_pool);
	rom->simulator->current_tick += 1;
	rom->process(rom);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0);

	return MUNIT_OK;
}

MunitTest rom_8d16a_tests[] = {
	{ "/read", test_read, rom_8d16a_setup, rom_8d16a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ce", test_ce, rom_8d16a_setup, rom_8d16a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
