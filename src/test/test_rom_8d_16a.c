// test/test_rom_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "rom_8d_16a.h"

static void *rom_8d16a_setup(const MunitParameter params[], void *user_data) {
	Rom8d16a *rom = rom_8d16a_create(16);

	for (uint32_t i = 0; i <= 0xffff; ++i) {
		rom->data_array[i] = i & 0xff;
	}
	return rom;
}

static void rom_8d16a_teardown(void *fixture) {
	Rom8d16a *rom = (Rom8d16a *) fixture;
	rom_8d16a_destroy(rom);
}

static MunitResult test_read(const MunitParameter params[], void *user_data_or_fixture) {

	Rom8d16a *rom = (Rom8d16a *) user_data_or_fixture;

	rom->pin_ce_b = ACTLO_ASSERT;

	for (uint32_t i = 0; i <= 0xffff; ++i) {
		rom->bus_address = i;
		rom_8d16a_process(rom);
		munit_assert_true(rom->upd_data);
		munit_assert_uint8(rom->bus_data, ==, i & 0xff);
	}

	return MUNIT_OK;
}

static MunitResult test_ce(const MunitParameter params[], void *user_data_or_fixture) {

	Rom8d16a *rom = (Rom8d16a *) user_data_or_fixture;
	
	rom->pin_ce_b = ACTLO_DEASSERT;
	rom->bus_address = 0x1635;
	rom_8d16a_process(rom);
	munit_assert_false(rom->upd_data);

	rom->pin_ce_b = ACTLO_ASSERT;
	rom_8d16a_process(rom);
	munit_assert_true(rom->upd_data);
	munit_assert_uint8(rom->bus_data, ==, 0x35);
	
	rom->pin_ce_b = ACTLO_DEASSERT;
	rom->bus_address = 0x12AF;
	rom_8d16a_process(rom);
	munit_assert_false(rom->upd_data);
	munit_assert_uint8(rom->bus_data, ==, 0x35);

	return MUNIT_OK;
}

MunitTest rom_8d16a_tests[] = {
	{ "/read", test_read, rom_8d16a_setup, rom_8d16a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ce", test_ce, rom_8d16a_setup, rom_8d16a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
