// test/test_main.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// main µnit entry point

#include "munit/munit.h"

extern MunitTest signal_tests[];
extern MunitTest cpu_6502_tests[];
extern MunitTest chip_6520_tests[];
extern MunitTest chip_6522_tests[];
extern MunitTest chip_74xxx_tests[];
extern MunitTest chip_hd44780_tests[];
extern MunitTest chip_mc3446a_tests[];
extern MunitTest chip_oscillator_tests[];
extern MunitTest chip_poweronreset_tests[];
extern MunitTest chip_ram_dynamic_tests[];
extern MunitTest chip_ram_static_tests[];
extern MunitTest chip_rom_tests[];
extern MunitTest simulator_tests[];
extern MunitTest dev_commodore_pet_tests[];
extern MunitTest dev_minimal_6502_tests[];
extern MunitTest input_keypad_tests[];
extern MunitTest ram_8d16a_tests[];
extern MunitTest rom_8d16a_tests[];
extern MunitTest utils_tests[];
extern MunitTest filt_6502_asm_tests[];
extern MunitTest signal_history_tests[];

static MunitSuite extern_suites[] = {
	{	.prefix = "/signal",
		.tests = signal_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/cpu_6502",
		.tests = cpu_6502_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/ram_8d16a",
		.tests = ram_8d16a_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/rom_8d16a",
		.tests = rom_8d16a_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/chip_6520",
		.tests = chip_6520_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/chip_6522",
		.tests = chip_6522_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/chip_74xxx",
		.tests = chip_74xxx_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/chip_hd44780",
		.tests = chip_hd44780_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/chip_mc3446a",
		.tests = chip_mc3446a_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/chip_oscillator",
		.tests = chip_oscillator_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/chip_poweronreset",
		.tests = chip_poweronreset_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/chip_ram_dynamic",
		.tests = chip_ram_dynamic_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/chip_ram_static",
		.tests = chip_ram_static_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/chip_rom",
		.tests = chip_rom_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/input_keypad",
		.tests = input_keypad_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/simulator",
		.tests = simulator_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/dev_commodore_pet",
		.tests = dev_commodore_pet_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/dev_minimal_6502",
		.tests = dev_minimal_6502_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/utils",
		.tests = utils_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/filt_6502_asm",
		.tests = filt_6502_asm_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{	.prefix = "/signal_history",
		.tests = signal_history_tests,
		.suites = NULL,
		.iterations = 1,
		.options = MUNIT_SUITE_OPTION_NONE
	},
	{ NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE}
};

static const MunitSuite suite = {
	"",
	NULL,		// tests
	extern_suites,
	1,			// iterations,
	MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char *argv[]) {
	return munit_suite_main_custom(&suite, NULL, argc, argv, NULL);
}
