// test/test_ram_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "ram_8d_16a.h"
#include "simulator.h"

#define SIGNAL_POOL			ram->signal_pool
#define SIGNAL_COLLECTION	ram->signals
#define SIGNAL_CHIP_ID		ram->id

static void *ram_8d16a_setup(const MunitParameter params[], void *user_data) {
	Ram8d16a *ram = ram_8d16a_create(16, simulator_create(NS_TO_PS(100)), (Ram8d16aSignals){0});
	return ram;
}

static void ram_8d16a_teardown(void *fixture) {
	Ram8d16a *ram = (Ram8d16a *) fixture;
	simulator_destroy(ram->simulator);
	ram_8d16a_destroy(ram);
}

MunitResult test_read(const MunitParameter params[], void *user_data_or_fixture) {

	Ram8d16a *ram = (Ram8d16a *) user_data_or_fixture;

	// initialize the memory
	for (uint32_t i = 0; i <= 0xffff; ++i) {
		ram->data_array[i] = i & 0xff;
	}

	// check memory
	for (uint32_t i = 0; i <= 0xffff; ++i) {
		SIGNAL_SET_BOOL(ce_b, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(oe_b, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(we_b, ACTLO_DEASSERT);
		SIGNAL_SET_UINT16(bus_address, i);
		signal_pool_cycle(ram->signal_pool, 1);

		ram_8d16a_process(ram);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, i & 0xff);
	}

	return MUNIT_OK;
}

MunitResult test_write(const MunitParameter params[], void *user_data_or_fixture) {

	Ram8d16a *ram = (Ram8d16a *) user_data_or_fixture;

	// write memory
	for (uint32_t i = 0; i <= 0xffff; ++i) {
		SIGNAL_SET_BOOL(ce_b, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(oe_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(we_b, ACTLO_ASSERT);
		SIGNAL_SET_UINT16(bus_address, i);
		SIGNAL_SET_UINT8(bus_data, i & 0xff);
		signal_pool_cycle(ram->signal_pool, 1);
		ram_8d16a_process(ram);

		SIGNAL_SET_BOOL(ce_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(we_b, ACTLO_DEASSERT);
		signal_pool_cycle(ram->signal_pool, 1);
		ram_8d16a_process(ram);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);
	}

	// check memory
	for (uint32_t i = 0; i <= 0xffff; ++i) {
		munit_assert_uint8(ram->data_array[i], ==, i & 0xff);
	}

	return MUNIT_OK;
}

MunitTest ram_8d16a_tests[] = {
	{ "/read", test_read, ram_8d16a_setup, ram_8d16a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write", test_write, ram_8d16a_setup, ram_8d16a_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
