// test/test_cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "cpu_6502.h"

typedef struct Computer {
	uint16_t	bus_address;
	uint8_t		bus_data;
	bool		pin_clock;
	bool		pin_reset;
	bool		pin_rw;

	Cpu6502 *	cpu;
} Computer;

MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {
	
	Computer computer = {
		.pin_reset = true,
		.pin_clock = true
	};
	computer.cpu = cpu_6502_create(
						&computer.bus_address, 
						&computer.bus_data,
						&computer.pin_clock,
						&computer.pin_reset,
						&computer.pin_rw);

	munit_assert_not_null(computer.cpu);

	/* deassert reset */
	cpu_6502_process(computer.cpu);
	computer.pin_reset = false;

	/* ignore first 5 cycles */
	for (int i = 0; i < 5; ++i) {
		computer.pin_clock = false;
		cpu_6502_process(computer.cpu);

		computer.pin_clock = true;
		cpu_6502_process(computer.cpu);
	}

	/* cpu should now read address 0xfffc - low byte of reset vector */
	computer.pin_clock = false;
	cpu_6502_process(computer.cpu);

	munit_assert_uint16(computer.bus_address, ==, 0xfffc);
	computer.bus_data = 0x01;

	computer.pin_clock = true;
	cpu_6502_process(computer.cpu);

	munit_assert_uint8(LOBYTE(computer.cpu->reg_pc), ==, 0x01);

	/* cpu should now read address 0xfffd - high byte of reset vector */
	computer.pin_clock = false;
	cpu_6502_process(computer.cpu);

	munit_assert_uint16(computer.bus_address, ==, 0xfffd);
	computer.bus_data = 0x08;

	computer.pin_clock = true;
	cpu_6502_process(computer.cpu);

	munit_assert_uint16(computer.cpu->reg_pc, ==, 0x0801);

	/* cpu should now read from address 0x0801 */
	computer.pin_clock = false;
	cpu_6502_process(computer.cpu);

	munit_assert_uint16(computer.bus_address, ==, 0x0801);

	return MUNIT_OK;
}

MunitTest cpu_6502_tests[] = {
	{ "/reset", test_reset, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
