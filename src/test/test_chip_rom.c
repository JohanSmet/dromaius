// test/test_chip_rom.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_rom.h"
#include "simulator.h"

#define SIGNAL_POOL			chip->signal_pool
#define SIGNAL_COLLECTION	chip->signals
#define SIGNAL_CHIP_ID		chip->id

static void fill_rom_8bit(uint8_t *data, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		data[i] = i & 0xff;
	}
}

static int ROM_63XX_ADDRESS_PINS[] = {
	CHIP_6332_A0, CHIP_6332_A1, CHIP_6332_A2, CHIP_6332_A3,
	CHIP_6332_A4, CHIP_6332_A5, CHIP_6332_A6, CHIP_6332_A7,
	CHIP_6332_A8, CHIP_6332_A9, CHIP_6332_A10, CHIP_6332_A11
};

static int ROM_63XX_DATA_PINS[] = {
	CHIP_6332_D0, CHIP_6332_D1, CHIP_6332_D2, CHIP_6332_D3,
	CHIP_6332_D4, CHIP_6332_D5, CHIP_6332_D6, CHIP_6332_D7
};

static inline void rom_63xx_set_address(Chip63xxRom *rom, uint16_t address, int n) {
	for (int i = 0; i < n; ++i) {
		signal_write_bool(rom->signal_pool, rom->signals[ROM_63XX_ADDRESS_PINS[i]], (address >> i) & 0x0001, rom->id);
	}
}

static inline uint8_t rom_63xx_read_data(Chip63xxRom *rom) {
	uint8_t result = 0;
	for (int i = 0; i < 8; ++i) {
		result |= signal_read_next_bool(rom->signal_pool, rom->signals[ROM_63XX_DATA_PINS[i]]) << i;
	}

	return result;
}

static inline void rom_6316_strobe(Chip63xxRom *rom, bool cs1_b, bool cs2_b, bool cs3) {
	signal_write_bool(rom->signal_pool, rom->signals[CHIP_6316_CS1_B], cs1_b, rom->id);
	signal_write_bool(rom->signal_pool, rom->signals[CHIP_6316_CS2_B], cs2_b, rom->id);
	signal_write_bool(rom->signal_pool, rom->signals[CHIP_6316_CS3], cs3, rom->id);
}

static inline void rom_6332_strobe(Chip63xxRom *rom, bool cs1_b, bool cs3) {
	signal_write_bool(rom->signal_pool, rom->signals[CHIP_6316_CS1_B], cs1_b, rom->id);
	signal_write_bool(rom->signal_pool, rom->signals[CHIP_6316_CS3], cs3, rom->id);
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
		rom_63xx_set_address(chip, i, 11);
		rom_63xx_cycle(chip);
		munit_assert_int64(chip->schedule_timestamp, !=, 0);

		chip->schedule_timestamp = 0;
		rom_63xx_cycle(chip);
		munit_assert_int64(chip->schedule_timestamp, ==, 0);
		munit_assert_uint8(rom_63xx_read_data(chip), ==, i & 0xff);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);

	return MUNIT_OK;
}

static MunitResult test_6316_cs(const MunitParameter params[], void *user_data_or_fixture) {

	Chip63xxRom *chip = chip_6316_rom_create(simulator_create(NS_TO_PS(100)), (Chip63xxSignals) {0});
	fill_rom_8bit(chip->data_array, chip->data_size);

	rom_6316_strobe(chip, ACTLO_DEASSERT, ACTLO_DEASSERT, ACTHI_DEASSERT);
	rom_63xx_set_address(chip, 0x0635, 11);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(rom_63xx_read_data(chip), ==, 0);

	rom_6316_strobe(chip, ACTLO_ASSERT, ACTLO_DEASSERT, ACTHI_DEASSERT);
	rom_63xx_set_address(chip, 0x0635, 11);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(rom_63xx_read_data(chip), ==, 0);

	rom_6316_strobe(chip, ACTLO_ASSERT, ACTLO_ASSERT, ACTHI_DEASSERT);
	rom_63xx_set_address(chip, 0x0635, 11);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(rom_63xx_read_data(chip), ==, 0);

	rom_6316_strobe(chip, ACTLO_ASSERT, ACTLO_ASSERT, ACTHI_ASSERT);
	rom_63xx_set_address(chip, 0x0635, 11);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(rom_63xx_read_data(chip), ==, 0x35);

	rom_6316_strobe(chip, ACTLO_DEASSERT, ACTLO_ASSERT, ACTHI_ASSERT);
	rom_63xx_set_address(chip, 0x0635, 11);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(rom_63xx_read_data(chip), ==, 0x0);

	simulator_destroy(chip->simulator);
	chip->destroy(chip);

	return MUNIT_OK;
}

static MunitResult test_6332_read(const MunitParameter params[], void *user_data_or_fixture) {

	Chip63xxRom *chip = chip_6332_rom_create(simulator_create(NS_TO_PS(100)), (Chip63xxSignals) {0});
	fill_rom_8bit(chip->data_array, chip->data_size);

	for (uint32_t i = 0; i <= chip->data_size; ++i) {
		rom_6332_strobe(chip, ACTLO_ASSERT, ACTHI_ASSERT);
		rom_63xx_set_address(chip, i, 12);
		rom_63xx_cycle(chip);
		munit_assert_int64(chip->schedule_timestamp, !=, 0);

		chip->schedule_timestamp = 0;
		rom_63xx_cycle(chip);
		munit_assert_int64(chip->schedule_timestamp, ==, 0);
		munit_assert_uint8(rom_63xx_read_data(chip), ==, i & 0xff);
	}

	simulator_destroy(chip->simulator);
	chip->destroy(chip);

	return MUNIT_OK;
}

static MunitResult test_6332_cs(const MunitParameter params[], void *user_data_or_fixture) {

	Chip63xxRom *chip = chip_6332_rom_create(simulator_create(NS_TO_PS(100)), (Chip63xxSignals) {0});
	fill_rom_8bit(chip->data_array, chip->data_size);

	rom_6332_strobe(chip, ACTLO_DEASSERT, ACTHI_DEASSERT);
	rom_63xx_set_address(chip, 0x0635, 11);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(rom_63xx_read_data(chip), ==, 0);

	rom_6332_strobe(chip, ACTLO_ASSERT, ACTHI_DEASSERT);
	rom_63xx_set_address(chip, 0x0635, 11);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(rom_63xx_read_data(chip), ==, 0);

	rom_6332_strobe(chip, ACTLO_ASSERT, ACTHI_ASSERT);
	rom_63xx_set_address(chip, 0x0635, 11);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(rom_63xx_read_data(chip), ==, 0x35);

	rom_6332_strobe(chip, ACTLO_DEASSERT, ACTHI_ASSERT);
	rom_63xx_set_address(chip, 0x0635, 11);
	rom_63xx_cycle(chip);
	rom_63xx_cycle(chip);
	munit_assert_uint8(rom_63xx_read_data(chip), ==, 0x0);

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
