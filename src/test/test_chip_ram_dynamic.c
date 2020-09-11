// test/test_chip_ram_dynamic.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_ram_dynamic.h"

#define SIGNAL_POOL			chip->signal_pool
#define SIGNAL_COLLECTION	chip->signals
#define SIGNAL_CHIP_ID		chip->id

static inline void ram_8x4116_cycle(Chip8x4116DRam *chip) {
	signal_pool_cycle(chip->signal_pool);
	chip->signal_pool->current_tick += 1;
	chip->process(chip);
}

MunitResult test_8x4116_read(const MunitParameter params[], void *user_data_or_fixture) {

	Chip8x4116DRam *chip = chip_8x4116_dram_create(signal_pool_create(), (Chip8x4116DRamSignals) {0});

	// initialize the memory
	for (uint32_t i = 0; i < 128; ++i) {
		for (uint32_t j = 0; j < 128; ++j) {
			chip->data_array[i * 128 + j] = (i + j) & 0xff;
		}
	}

	// check memory
	for (uint32_t i = 0; i < 0x4000; ++i) {
		int row = (i >> 7) & 0x7f;
		int col = i & 0x7f;
		int expected_data = (row + col) & 0xff;

		SIGNAL_SET_BOOL(ras_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cas_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(we_b, ACTLO_DEASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// set row address
		SIGNAL_SET_UINT8(bus_address, row);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// assert ras_b
		SIGNAL_SET_BOOL(ras_b, ACTLO_ASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// set col address
		SIGNAL_SET_UINT8(bus_address, col);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// assert cas_b
		SIGNAL_SET_BOOL(cas_b, ACTLO_ASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_int(chip->col, ==, col);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// check output
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, expected_data);

		// deassert cas_b
		SIGNAL_SET_BOOL(cas_b, ACTLO_DEASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_int(chip->col, ==, col);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);
	}

	chip->destroy(chip);
	signal_pool_destroy(chip->signal_pool);

	return MUNIT_OK;
}

MunitResult test_8x4116_write(const MunitParameter params[], void *user_data_or_fixture) {

	Chip8x4116DRam *chip = chip_8x4116_dram_create(signal_pool_create(), (Chip8x4116DRamSignals) {0});

	// initialize the memory
	for (uint32_t i = 0; i < 128; ++i) {
		for (uint32_t j = 0; j < 128; ++j) {
			chip->data_array[i * 128 + j] = (i + j) & 0xff;
		}
	}

	// write memory
	uint8_t write_value[] = {0xaa, 0x55};

	for (uint32_t i = 0; i < 0x4000; ++i) {
		int row = (i >> 7) & 0x7f;
		int col = i & 0x7f;
		int expected_data = (row + col) & 0xff;

		SIGNAL_SET_BOOL(ras_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cas_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(we_b, ACTLO_DEASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// set row address
		SIGNAL_SET_UINT8(bus_address, row);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// assert ras_b
		SIGNAL_SET_BOOL(ras_b, ACTLO_ASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// set col address
		SIGNAL_SET_UINT8(bus_address, col);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// assert cas_b
		SIGNAL_SET_BOOL(cas_b, ACTLO_ASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_int(chip->col, ==, col);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// check output
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, expected_data);

		// start writing to di
		SIGNAL_SET_UINT8(bus_di, write_value[col & 0x01]);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, expected_data);

		// assert we_b
		SIGNAL_SET_BOOL(we_b, ACTLO_ASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, expected_data);
		munit_assert_uint8(chip->data_array[row * 128 + col], ==, write_value[col & 0x01]);

		// deassert we_b
		SIGNAL_SET_BOOL(we_b, ACTLO_DEASSERT);
		SIGNAL_NO_WRITE(bus_di);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, expected_data);

		// deassert cas_b
		SIGNAL_SET_BOOL(cas_b, ACTLO_DEASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_int(chip->col, ==, col);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);
	}

	// check memory
	for (uint32_t i = 0; i < 128; ++i) {
		for (uint32_t j = 0; j < 128; ++j) {
			munit_assert_uint8(chip->data_array[i * 128 + j], ==, write_value[j & 0x01]);
		}
	}

	chip->destroy(chip);
	signal_pool_destroy(chip->signal_pool);
	return MUNIT_OK;
}


MunitResult test_8x4116_early_write(const MunitParameter params[], void *user_data_or_fixture) {

	Chip8x4116DRam *chip = chip_8x4116_dram_create(signal_pool_create(), (Chip8x4116DRamSignals) {0});

	// initialize the memory
	for (uint32_t i = 0; i < 128; ++i) {
		for (uint32_t j = 0; j < 128; ++j) {
			chip->data_array[i * 128 + j] = (i + j) & 0xff;
		}
	}

	// write memory
	uint8_t write_value[] = {0xaa, 0x55};

	for (uint32_t i = 0; i < 0x4000; ++i) {
		int row = (i >> 7) & 0x7f;
		int col = i & 0x7f;

		SIGNAL_SET_BOOL(ras_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cas_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(we_b, ACTLO_DEASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// set row address
		SIGNAL_SET_UINT8(bus_address, row);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// assert ras_b
		SIGNAL_SET_BOOL(ras_b, ACTLO_ASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// assert we_b
		SIGNAL_SET_BOOL(we_b, ACTLO_ASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// start writing to di
		SIGNAL_SET_UINT8(bus_di, write_value[col & 0x01]);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// set col address
		SIGNAL_SET_UINT8(bus_address, col);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// assert cas_b
		SIGNAL_SET_BOOL(cas_b, ACTLO_ASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_int(chip->col, ==, col);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);
		munit_assert_uint8(chip->data_array[row * 128 + col], ==, write_value[col & 0x01]);

		// deassert we_b
		SIGNAL_SET_BOOL(we_b, ACTLO_DEASSERT);
		SIGNAL_NO_WRITE(bus_di);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// deassert cas_b & ras_b
		SIGNAL_SET_BOOL(ras_b, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cas_b, ACTLO_DEASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_int(chip->col, ==, col);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);
	}

	// check memory
	for (uint32_t i = 0; i < 128; ++i) {
		for (uint32_t j = 0; j < 128; ++j) {
			munit_assert_uint8(chip->data_array[i * 128 + j], ==, write_value[j & 0x01]);
		}
	}

	chip->destroy(chip);
	signal_pool_destroy(chip->signal_pool);
	return MUNIT_OK;
}

MunitResult test_8x4116_page_mode_read(const MunitParameter params[], void *user_data_or_fixture) {

	Chip8x4116DRam *chip = chip_8x4116_dram_create(signal_pool_create(), (Chip8x4116DRamSignals) {0});

	// initialize the memory
	for (uint32_t i = 0; i < 128; ++i) {
		for (uint32_t j = 0; j < 128; ++j) {
			chip->data_array[i * 128 + j] = (i + j) & 0xff;
		}
	}

	// init chip
	SIGNAL_SET_BOOL(ras_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(cas_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(we_b, ACTLO_DEASSERT);
	ram_8x4116_cycle(chip);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

	// check memory
	for (int row = 0; row < 128; ++row) {

		// set row address
		SIGNAL_SET_UINT8(bus_address, row);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// assert ras_b
		SIGNAL_SET_BOOL(ras_b, ACTLO_ASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		for (uint32_t col = 0; col < 128; ++col) {
			int expected_data = (row + col) & 0xff;

			// set col address
			SIGNAL_SET_UINT8(bus_address, col);
			ram_8x4116_cycle(chip);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

			// assert cas_b
			SIGNAL_SET_BOOL(cas_b, ACTLO_ASSERT);
			ram_8x4116_cycle(chip);
			munit_assert_int(chip->row, ==, row);
			munit_assert_int(chip->col, ==, col);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

			// check output
			ram_8x4116_cycle(chip);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, expected_data);

			// deassert cas_b
			SIGNAL_SET_BOOL(cas_b, ACTLO_DEASSERT);
			ram_8x4116_cycle(chip);
			munit_assert_int(chip->row, ==, row);
			munit_assert_int(chip->col, ==, col);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);
		}

		// deassert ras_b
		SIGNAL_SET_BOOL(ras_b, ACTLO_DEASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);
	}

	chip->destroy(chip);
	signal_pool_destroy(chip->signal_pool);

	return MUNIT_OK;
}

MunitResult test_8x4116_page_mode_write(const MunitParameter params[], void *user_data_or_fixture) {

	Chip8x4116DRam *chip = chip_8x4116_dram_create(signal_pool_create(), (Chip8x4116DRamSignals) {0});

	// initialize the memory
	for (uint32_t i = 0; i < 128; ++i) {
		for (uint32_t j = 0; j < 128; ++j) {
			chip->data_array[i * 128 + j] = (i + j) & 0xff;
		}
	}

	// init chip
	SIGNAL_SET_BOOL(ras_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(cas_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(we_b, ACTLO_DEASSERT);
	ram_8x4116_cycle(chip);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

	// write memory
	uint8_t write_value[] = {0xaa, 0x55};

	for (int row = 0; row < 128; ++row) {

		// set row address
		SIGNAL_SET_UINT8(bus_address, row);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		// assert ras_b
		SIGNAL_SET_BOOL(ras_b, ACTLO_ASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_int(chip->row, ==, row);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

		for (int col = 0; col < 128; ++col) {

			// assert we_b
			SIGNAL_SET_BOOL(we_b, ACTLO_ASSERT);
			ram_8x4116_cycle(chip);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

			// start writing to di
			SIGNAL_SET_UINT8(bus_di, write_value[col & 0x01]);
			ram_8x4116_cycle(chip);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

			// set col address
			SIGNAL_SET_UINT8(bus_address, col);
			ram_8x4116_cycle(chip);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

			// assert cas_b
			SIGNAL_SET_BOOL(cas_b, ACTLO_ASSERT);
			ram_8x4116_cycle(chip);
			munit_assert_int(chip->row, ==, row);
			munit_assert_int(chip->col, ==, col);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);
			munit_assert_uint8(chip->data_array[row * 128 + col], ==, write_value[col & 0x01]);

			// deassert we_b
			SIGNAL_SET_BOOL(we_b, ACTLO_DEASSERT);
			SIGNAL_NO_WRITE(bus_di);
			ram_8x4116_cycle(chip);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);

			// deassert cas_b
			SIGNAL_SET_BOOL(cas_b, ACTLO_DEASSERT);
			ram_8x4116_cycle(chip);
			munit_assert_int(chip->row, ==, row);
			munit_assert_int(chip->col, ==, col);
			munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);
		}

		// deassert ras_b
		SIGNAL_SET_BOOL(ras_b, ACTLO_DEASSERT);
		ram_8x4116_cycle(chip);
		munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_do), ==, 0);
	}

	// check memory
	for (uint32_t i = 0; i < 128; ++i) {
		for (uint32_t j = 0; j < 128; ++j) {
			munit_assert_uint8(chip->data_array[i * 128 + j], ==, write_value[j & 0x01]);
		}
	}

	chip->destroy(chip);
	signal_pool_destroy(chip->signal_pool);
	return MUNIT_OK;
}

MunitTest chip_ram_dynamic_tests[] = {
	{ "/8x4116_read", test_8x4116_read, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/8x4116_write", test_8x4116_write, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/8x4116_early_write", test_8x4116_early_write, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/8x4116_page_mode_read", test_8x4116_page_mode_read, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/8x4116_page_mode_write", test_8x4116_page_mode_write, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
