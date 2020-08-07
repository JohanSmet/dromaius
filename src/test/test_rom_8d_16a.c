// test/test_rom_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "rom_8d_16a.h"

#define SIGNAL_POOL			rom->signal_pool
#define SIGNAL_COLLECTION	rom->signals
#define SIGNAL_CHIP_ID		rom->id

static void *rom_8d16a_setup(const MunitParameter params[], void *user_data) {
	SignalPool *pool = signal_pool_create(1);
	Rom8d16a *rom = rom_8d16a_create(16, pool, (Rom8d16aSignals) {0});

	for (uint32_t i = 0; i <= 0xffff; ++i) {
		rom->data_array[i] = i & 0xff;
	}
	return rom;
}

static void rom_8d16a_teardown(void *fixture) {
	Rom8d16a *rom = (Rom8d16a *) fixture;
	signal_pool_destroy(rom->signal_pool);
	rom_8d16a_destroy(rom);
}

static MunitResult test_read(const MunitParameter params[], void *user_data_or_fixture) {

	Rom8d16a *rom = (Rom8d16a *) user_data_or_fixture;

	for (uint32_t i = 0; i <= 0xffff; ++i) {
		SIGNAL_SET_BOOL(ce_b, ACTLO_ASSERT);
		SIGNAL_SET_UINT16(bus_address, i);
		signal_pool_cycle(rom->signal_pool);

		rom_8d16a_process(rom);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, i & 0xff);
	}

	return MUNIT_OK;
}

static MunitResult test_ce(const MunitParameter params[], void *user_data_or_fixture) {

	Rom8d16a *rom = (Rom8d16a *) user_data_or_fixture;

	SIGNAL_SET_BOOL(ce_b, ACTLO_DEASSERT);
	SIGNAL_SET_UINT16(bus_address, 0x1635);
	signal_pool_cycle(rom->signal_pool);
	rom_8d16a_process(rom);
	signal_pool_cycle(rom->signal_pool);
	munit_assert_uint8(SIGNAL_UINT8(bus_data), ==, 0);

	SIGNAL_SET_BOOL(ce_b, ACTLO_ASSERT);
	SIGNAL_SET_UINT16(bus_address, 0x1635);
	signal_pool_cycle(rom->signal_pool);
	rom_8d16a_process(rom);
	signal_pool_cycle(rom->signal_pool);
	munit_assert_uint8(SIGNAL_UINT8(bus_data), ==, 0x35);

	SIGNAL_SET_BOOL(ce_b, ACTLO_DEASSERT);
	SIGNAL_SET_UINT16(bus_address, 0x12AF);
	signal_pool_cycle(rom->signal_pool);
	rom_8d16a_process(rom);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0);

	return MUNIT_OK;
}

MunitTest rom_8d16a_tests[] = {
	{ "/read", test_read, rom_8d16a_setup, rom_8d16a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ce", test_ce, rom_8d16a_setup, rom_8d16a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
