// test/test_ram_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "ram_8d_16a.h"

static void *ram_8d16a_setup(const MunitParameter params[], void *user_data) {
	Ram8d16a *ram = ram_8d16a_create(16);
	return ram;
}

static void ram_8d16a_teardown(void *fixture) {
	Ram8d16a *ram = (Ram8d16a *) fixture;
	ram_8d16a_destroy(ram);
}

MunitResult test_address_mask(const MunitParameter params[], void *user_data_or_fixture) {
	
	// >> 1 address line
	Ram8d16a *ram_1a = ram_8d16a_create(1);
	munit_assert_uint16(ram_1a->msk_address, ==, 0x0001);
	ram_8d16a_destroy(ram_1a);

	// >> 2 address lines
	Ram8d16a *ram_2a = ram_8d16a_create(2);
	munit_assert_uint16(ram_1a->msk_address, ==, 0x0003);
	ram_8d16a_destroy(ram_1a);

	// >> 8 address lines
	Ram8d16a *ram_8a = ram_8d16a_create(8);
	munit_assert_uint16(ram_1a->msk_address, ==, 0x00ff);
	ram_8d16a_destroy(ram_1a);

	// >> 16 address lines
	Ram8d16a *ram_16a = ram_8d16a_create(16);
	munit_assert_uint16(ram_1a->msk_address, ==, 0xffff);
	ram_8d16a_destroy(ram_1a);

	return MUNIT_OK;
}

MunitResult test_read(const MunitParameter params[], void *user_data_or_fixture) {

	Ram8d16a *ram = (Ram8d16a *) user_data_or_fixture;

	// initialize the memory
	for (uint32_t i = 0; i <= 0xffff; ++i) {
		ram->data_array[i] = i & 0xff;
	}

	ram->pin_ce_b = ACTLO_ASSERT;
	ram->pin_oe_b = ACTLO_ASSERT;
	ram->pin_we_b = ACTLO_DEASSERT;

	// check memory
	for (uint32_t i = 0; i <= 0xffff; ++i) {
		ram->bus_address = i;
		ram_8d16a_process(ram);
		munit_assert_true(ram->upd_data);
		munit_assert_uint8(ram->bus_data, ==, i & 0xff);
	}

	return MUNIT_OK;
}

MunitResult test_write(const MunitParameter params[], void *user_data_or_fixture) {

	Ram8d16a *ram = (Ram8d16a *) user_data_or_fixture;

	ram->pin_ce_b = ACTLO_ASSERT;
	ram->pin_oe_b = ACTLO_DEASSERT;
	ram->pin_we_b = ACTLO_ASSERT;

	// write memory
	for (uint32_t i = 0; i <= 0xffff; ++i) {
		ram->bus_address = i;
		ram->bus_data = i & 0xff;
		ram_8d16a_process(ram);
		munit_assert_false(ram->upd_data);
	}

	// check memory
	for (uint32_t i = 0; i <= 0xffff; ++i) {
		munit_assert_uint8(ram->data_array[i], ==, i & 0xff);
	}

	return MUNIT_OK;
}

MunitTest ram_8d16a_tests[] = {
	{ "/msk_address", test_address_mask, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read", test_read, ram_8d16a_setup, ram_8d16a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write", test_write, ram_8d16a_setup, ram_8d16a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
