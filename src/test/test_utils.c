// test/test_utils.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"

#include "utils.h"
#include "stb/stb_ds.h"

MunitResult test_printf(const MunitParameter params[], void* user_data_or_fixture) {
    char *test_string = NULL;

    arr_printf(test_string, "%d", 10);
    munit_assert_string_equal(test_string, "10");
    arr_printf(test_string, " %.2f", 3.14);
    munit_assert_string_equal(test_string, "10 3.14");

	arrsetlen(test_string, 0);
    arr_printf(test_string, "Hello World");
    munit_assert_string_equal(test_string, "Hello World");

    arrfree(test_string); 

    return MUNIT_OK;
}

MunitTest utils_tests[] = {
    { "/printf", test_printf, NULL, NULL,  MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
