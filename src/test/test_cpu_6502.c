// test/test_cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "cpu_6502.h"

MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {
	return MUNIT_OK;
}

MunitTest cpu_6502_tests[] = {
	{ "/reset", test_reset, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
