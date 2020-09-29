// test/test_device.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "simulator.h"

#include "stb/stb_ds.h"

MunitResult test_schedule_event(const MunitParameter params[], void *user_data_or_fixture) {

	Simulator *simulator = simulator_create(NS_TO_PS(100));

	munit_assert_ptr_not_null(simulator);
	munit_assert_ptr_null(simulator->event_schedule);

	simulator_schedule_event(simulator, 3, 250);
	munit_assert_ptr_not_null(simulator->event_schedule);
	munit_assert_uint32(simulator->event_schedule->chip_id, ==, 3);
	munit_assert_uint64(simulator->event_schedule->timestamp, ==, 250);
	munit_assert_ptr_null(simulator->event_schedule->next);

	simulator_schedule_event(simulator, 1, 150);
	munit_assert_ptr_not_null(simulator->event_schedule);
	{
		ChipEvent *first = simulator->event_schedule;
		munit_assert_uint32(first->chip_id, ==, 1);
		munit_assert_uint64(first->timestamp, ==, 150);
		munit_assert_ptr_not_null(first->next);

		ChipEvent *second = first->next;
		munit_assert_uint32(second->chip_id, ==, 3);
		munit_assert_uint64(second->timestamp, ==, 250);
		munit_assert_ptr_null(second->next);
	}

	simulator_schedule_event(simulator, 2, 200);
	munit_assert_ptr_not_null(simulator->event_schedule);
	{
		ChipEvent *first = simulator->event_schedule;
		munit_assert_uint32(first->chip_id, ==, 1);
		munit_assert_uint64(first->timestamp, ==, 150);
		munit_assert_ptr_not_null(first->next);

		ChipEvent *second = first->next;
		munit_assert_uint32(second->chip_id, ==, 2);
		munit_assert_uint64(second->timestamp, ==, 200);
		munit_assert_ptr_not_null(second->next);

		ChipEvent *third = second->next;
		munit_assert_uint32(third->chip_id, ==, 3);
		munit_assert_uint64(third->timestamp, ==, 250);
		munit_assert_ptr_null(third->next);
	}

	simulator_destroy(simulator);
	return MUNIT_OK;
}

MunitResult test_pop_event(const MunitParameter params[], void *user_data_or_fixture) {
	Simulator *simulator = simulator_create(NS_TO_PS(100));
	munit_assert_ptr_not_null(simulator);

	simulator_schedule_event(simulator, 3, 250);
	simulator_schedule_event(simulator, 1, 150);
	simulator_schedule_event(simulator, 2, 200);

	munit_assert_int64(simulator->event_schedule->timestamp, ==, 150);

	munit_assert_int32(simulator_pop_scheduled_event(simulator, 100), ==, -1);

	munit_assert_int32(simulator_pop_scheduled_event(simulator, 150), ==, 1);
	munit_assert_int32(simulator_pop_scheduled_event(simulator, 150), ==, -1);
	munit_assert_int64(simulator->event_schedule->timestamp, ==, 200);

	munit_assert_int32(simulator_pop_scheduled_event(simulator, 200), ==, 2);
	munit_assert_int32(simulator_pop_scheduled_event(simulator, 200), ==, -1);
	munit_assert_int64(simulator->event_schedule->timestamp, ==, 250);

	munit_assert_int32(simulator_pop_scheduled_event(simulator, 250), ==, 3);
	munit_assert_int32(simulator_pop_scheduled_event(simulator, 250), ==, -1);
	munit_assert_ptr_null(simulator->event_schedule);

	simulator_destroy(simulator);
	return MUNIT_OK;
}

MunitTest simulator_tests[] = {
	{ "/schedule_event", test_schedule_event, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/pop_event", test_pop_event, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
