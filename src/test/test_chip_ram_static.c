// test/test_chip_ram_static.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_ram_static.h"
#include "simulator.h"

#define SIGNAL_POOL			chip->signal_pool
#define SIGNAL_COLLECTION	chip->signals
#define SIGNAL_CHIP_ID		chip->id

MunitResult test_6114_read(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6114SRam *chip = chip_6114_sram_create(simulator_create(NS_TO_PS(100)), (Chip6114SRamSignals) {0});

	// initialize the memory
	for (uint32_t i = 0; i < 1024; ++i) {
		chip->data_array[i] = i & 0x0f;
	}

	// check memory
	for (uint32_t i = 0; i < 1024; ++i) {
		SIGNAL_SET_BOOL(ce_b, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(rw, true);
		SIGNAL_SET_UINT16(bus_address, i);
		signal_pool_cycle(chip->signal_pool, 1);

		chip_6114_sram_process(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_io), ==, i & 0x0f);
	}

	chip_6114_sram_destroy(chip);

	return MUNIT_OK;
}

MunitResult test_6114_write(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6114SRam *chip = chip_6114_sram_create(simulator_create(NS_TO_PS(100)), (Chip6114SRamSignals) {0});

	// write memory
	for (uint32_t i = 0; i < 1024; ++i) {
		SIGNAL_SET_BOOL(ce_b, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_UINT16(bus_address, i);
		SIGNAL_SET_UINT8(bus_io, i & 0x0f);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_6114_sram_process(chip);

		SIGNAL_SET_BOOL(ce_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(rw, true);
		signal_pool_cycle(chip->signal_pool, 1);
		chip_6114_sram_process(chip);
		signal_pool_cycle(chip->signal_pool, 1);

		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_io), ==, 0x00);
	}

	// check memory
	for (uint32_t i = 0; i < 1024; ++i) {
		munit_assert_uint8(chip->data_array[i], ==, i & 0x0f);
	}

	chip_6114_sram_destroy(chip);
	return MUNIT_OK;
}

MunitTest chip_ram_static_tests[] = {
	{ "/6114_read", test_6114_read, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/6144_write", test_6114_write, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
