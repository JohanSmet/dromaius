// test/test_chip_rom.c - Johan Smet - BSD-3-Clause (see LICENSE)

#define SIGNAL_ARRAY_STYLE
#include "munit/munit.h"
#include "chip_rom.h"
#include "simulator.h"

#define SIGNAL_OWNER	chip
#define SIGNAL_PREFIX

static void fill_rom_8bit(uint8_t *data, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		data[i] = i & 0xff;
	}
}

static inline void rom_6316_strobe(Chip63xxRom *chip, bool cs1_b, bool cs2_b, bool cs3) {
	SIGNAL_WRITE(CHIP_6316_CS1_B, cs1_b);
	SIGNAL_WRITE(CHIP_6316_CS2_B, cs2_b);
	SIGNAL_WRITE(CHIP_6316_CS3,   cs3);
}

static inline void rom_6332_strobe(Chip63xxRom *chip, bool cs1_b, bool cs3) {
	SIGNAL_WRITE(CHIP_6332_CS1_B, cs1_b);
	SIGNAL_WRITE(CHIP_6332_CS3,   cs3);
}

static inline void rom_63xx_cycle(Chip63xxRom *chip) {
	signal_pool_cycle(chip->signal_pool, chip->simulator->current_tick);
	chip->simulator->current_tick += 1;
	chip->process(chip);
}

//////////////////////////////////////////////////////////////////////////////
//
// tests
//

static MunitResult test_6316_read(const MunitParameter params[], void *user_data_or_fixture) {

	Chip63xxRom *chip = chip_6316_rom_create(simulator_create(NS_TO_PS(100)), (Chip63xxSignals) {0});
	fill_rom_8bit(chip->data_array, chip->data_size);

	for (uint32_t i = 0; i <= ROM_6316_DATA_SIZE; ++i) {
		rom_6316_strobe(chip, ACTLO_ASSERT, ACTLO_ASSERT, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(address, i);
		rom_63xx_cycle(chip);
		munit_assert_int64(chip->schedule_timestamp, !=, 0);

		chip->schedule_timestamp = 0;
		rom_63xx_cycle(chip);
		munit_assert_int64(chip->schedule_timestamp, ==, 0);
		munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, i & 0xff);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);

	return MUNIT_OK;
}

static MunitResult test_6316_cs(const MunitParameter params[], void *user_data_or_fixture) {

	Chip63xxRom *chip = chip_6316_rom_create(simulator_create(NS_TO_PS(100)), (Chip63xxSignals) {0});
	fill_rom_8bit(chip->data_array, chip->data_size);

	rom_6316_strobe(chip, ACTLO_DEASSERT, ACTLO_DEASSERT, ACTHI_DEASSERT);
	SIGNAL_GROUP_WRITE(address, 0x0635);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0);

	rom_6316_strobe(chip, ACTLO_ASSERT, ACTLO_DEASSERT, ACTHI_DEASSERT);
	SIGNAL_GROUP_WRITE(address, 0x0635);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0);

	rom_6316_strobe(chip, ACTLO_ASSERT, ACTLO_ASSERT, ACTHI_DEASSERT);
	SIGNAL_GROUP_WRITE(address, 0x0635);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0);

	rom_6316_strobe(chip, ACTLO_ASSERT, ACTLO_ASSERT, ACTHI_ASSERT);
	SIGNAL_GROUP_WRITE(address, 0x0635);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0x35);

	rom_6316_strobe(chip, ACTLO_DEASSERT, ACTLO_ASSERT, ACTHI_ASSERT);
	SIGNAL_GROUP_WRITE(address, 0x0635);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0x0);

	simulator_destroy(chip->simulator);
	chip->destroy(chip);

	return MUNIT_OK;
}

static MunitResult test_6332_read(const MunitParameter params[], void *user_data_or_fixture) {

	Chip63xxRom *chip = chip_6332_rom_create(simulator_create(NS_TO_PS(100)), (Chip63xxSignals) {0});
	fill_rom_8bit(chip->data_array, chip->data_size);

	for (uint32_t i = 0; i <= chip->data_size; ++i) {
		rom_6332_strobe(chip, ACTLO_ASSERT, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(address, i);
		rom_63xx_cycle(chip);
		munit_assert_int64(chip->schedule_timestamp, !=, 0);

		chip->schedule_timestamp = 0;
		rom_63xx_cycle(chip);
		munit_assert_int64(chip->schedule_timestamp, ==, 0);
		munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, i & 0xff);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);

	return MUNIT_OK;
}

static MunitResult test_6332_cs(const MunitParameter params[], void *user_data_or_fixture) {

	Chip63xxRom *chip = chip_6332_rom_create(simulator_create(NS_TO_PS(100)), (Chip63xxSignals) {0});
	fill_rom_8bit(chip->data_array, chip->data_size);

	rom_6332_strobe(chip, ACTLO_DEASSERT, ACTHI_DEASSERT);
	SIGNAL_GROUP_WRITE(address, 0x0635);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0);

	rom_6332_strobe(chip, ACTLO_ASSERT, ACTHI_DEASSERT);
	SIGNAL_GROUP_WRITE(address, 0x0635);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0);

	rom_6332_strobe(chip, ACTLO_ASSERT, ACTHI_ASSERT);
	SIGNAL_GROUP_WRITE(address, 0x0635);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0x35);

	rom_6332_strobe(chip, ACTLO_DEASSERT, ACTHI_ASSERT);
	SIGNAL_GROUP_WRITE(address, 0x0635);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0x0);

	simulator_destroy(chip->simulator);
	chip->destroy(chip);

	return MUNIT_OK;
}

MunitTest chip_rom_tests[] = {
	{ "/6316_read", test_6316_read, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/6316_cs", test_6316_cs, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/6332_read", test_6332_read, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/6332_cs", test_6332_cs, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
