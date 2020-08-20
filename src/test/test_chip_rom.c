// test/test_chip_rom.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_rom.h"

#define SIGNAL_POOL			chip->signal_pool
#define SIGNAL_COLLECTION	chip->signals
#define SIGNAL_CHIP_ID		chip->id

static void fill_rom_8bit(uint8_t *data, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		data[i] = i & 0xff;
	}
}

#define ROM_READ_8BIT()		((SIGNAL_NEXT_BOOL(d0) << 0) | (SIGNAL_NEXT_BOOL(d1) << 1) |	\
							 (SIGNAL_NEXT_BOOL(d2) << 2) | (SIGNAL_NEXT_BOOL(d3) << 3) |	\
							 (SIGNAL_NEXT_BOOL(d4) << 4) | (SIGNAL_NEXT_BOOL(d5) << 5) |	\
							 (SIGNAL_NEXT_BOOL(d6) << 6) | (SIGNAL_NEXT_BOOL(d7) << 7))

#define ROM_SET_ADDRESS_11(a)	SIGNAL_SET_BOOL(a0, ((a) >> 0) & 0x1);	\
								SIGNAL_SET_BOOL(a1, ((a) >> 1) & 0x1);	\
								SIGNAL_SET_BOOL(a2, ((a) >> 2) & 0x1);	\
								SIGNAL_SET_BOOL(a3, ((a) >> 3) & 0x1);	\
								SIGNAL_SET_BOOL(a4, ((a) >> 4) & 0x1);	\
								SIGNAL_SET_BOOL(a5, ((a) >> 5) & 0x1);	\
								SIGNAL_SET_BOOL(a6, ((a) >> 6) & 0x1);	\
								SIGNAL_SET_BOOL(a7, ((a) >> 7) & 0x1);	\
								SIGNAL_SET_BOOL(a8, ((a) >> 8) & 0x1);	\
								SIGNAL_SET_BOOL(a9, ((a) >> 9) & 0x1);	\
								SIGNAL_SET_BOOL(a10, ((a) >> 10) & 0x1);

static MunitResult test_6316_read(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6316Rom *chip = chip_6316_rom_create(signal_pool_create(), (Chip6316Signals) {0});
	fill_rom_8bit(chip->data_array, ROM_6316_DATA_SIZE);

	for (uint32_t i = 0; i <= ROM_6316_DATA_SIZE; ++i) {
		SIGNAL_SET_BOOL(cs1_b, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(cs2_b, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(cs3, ACTHI_ASSERT);
		ROM_SET_ADDRESS_11(i);
		signal_pool_cycle(chip->signal_pool);

		chip->process(chip);
		munit_assert_uint8(ROM_READ_8BIT(), ==, i);
	}

	signal_pool_destroy(chip->signal_pool);
	chip_6316_rom_destroy(chip);

	return MUNIT_OK;
}

static MunitResult test_6316_cs(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6316Rom *chip = chip_6316_rom_create(signal_pool_create(), (Chip6316Signals) {0});
	fill_rom_8bit(chip->data_array, ROM_6316_DATA_SIZE);

	SIGNAL_SET_BOOL(cs1_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(cs2_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(cs3,   ACTHI_DEASSERT);
	ROM_SET_ADDRESS_11(0x0635);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_uint8(ROM_READ_8BIT(), ==, 0);

	SIGNAL_SET_BOOL(cs1_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(cs2_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(cs3,   ACTHI_DEASSERT);
	ROM_SET_ADDRESS_11(0x0635);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_uint8(ROM_READ_8BIT(), ==, 0);

	SIGNAL_SET_BOOL(cs1_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(cs2_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(cs3,   ACTHI_DEASSERT);
	ROM_SET_ADDRESS_11(0x0635);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_uint8(ROM_READ_8BIT(), ==, 0);

	SIGNAL_SET_BOOL(cs1_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(cs2_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(cs3,   ACTHI_ASSERT);
	ROM_SET_ADDRESS_11(0x0635);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_uint8(ROM_READ_8BIT(), ==, 0x35);

	SIGNAL_SET_BOOL(cs1_b, ACTLO_DEASSERT);
	SIGNAL_SET_BOOL(cs2_b, ACTLO_ASSERT);
	SIGNAL_SET_BOOL(cs3,   ACTHI_ASSERT);
	ROM_SET_ADDRESS_11(0x0635);
	signal_pool_cycle(chip->signal_pool);
	chip->process(chip);
	munit_assert_uint8(ROM_READ_8BIT(), ==, 0x0);

	signal_pool_destroy(chip->signal_pool);
	chip_6316_rom_destroy(chip);

	return MUNIT_OK;
}

MunitTest chip_rom_tests[] = {
	{ "/6316_read", test_6316_read, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/6316_cs", test_6316_cs, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
