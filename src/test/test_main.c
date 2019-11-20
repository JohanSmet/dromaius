// test/test_main.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// main µnit entry point

#include "munit/munit.h"

extern MunitTest cpu_6502_tests[];
extern MunitTest dev_minimal_6502_tests[];
extern MunitTest ram_8d16a_tests[];
extern MunitTest rom_8d16a_tests[];
extern MunitTest utils_tests[];
extern MunitTest filt_6502_asm_tests[];

static MunitSuite extern_suites[] = {
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
