// test/test_device.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "simulator.h"

#include "stb/stb_ds.h"

MunitResult test_schedule_event(const MunitParameter params[], void *user_data_or_fixture) {

	Simulator *simulator = simulator_create(NS_TO_PS(100));

	munit_assert_ptr_not_null(simulator);
	munit_assert_ptr_not_null(simulator->event_schedule);
	munit_assert_ptr_not_null(simulator->event_schedule[0]);
	{
		ChipEvent *sentinel = simulator->event_schedule[0];
		munit_assert_uint64(sentinel->chip_mask, ==, 0);
		munit_assert_int64(sentinel->timestamp, ==, INT64_MAX);
		munit_assert_ptr_null(sentinel->next);
	}

	simulator_schedule_event(simulator, 3, 250);
	munit_assert_ptr_not_null(simulator->event_schedule[0]);
	{
		ChipEvent *first = simulator->event_schedule[0];
		munit_assert_uint64(first->chip_mask, ==, 0b1000);
		munit_assert_int64(first->timestamp, ==, 250);
		munit_assert_ptr_not_null(first->next);

		ChipEvent *sentinel = first->next;
		munit_assert_uint64(sentinel->chip_mask, ==, 0);
		munit_assert_int64(sentinel->timestamp, ==, INT64_MAX);
		munit_assert_ptr_null(sentinel->next);
	}

	simulator_schedule_event(simulator, 1, 150);
	munit_assert_ptr_not_null(simulator->event_schedule[0]);
	{
		ChipEvent *first = simulator->event_schedule[0];
		munit_assert_uint64(first->chip_mask, ==, 0b0010);
		munit_assert_int64(first->timestamp, ==, 150);
		munit_assert_ptr_not_null(first->next);

		ChipEvent *second = first->next;
		munit_assert_uint64(second->chip_mask, ==, 0b1000);
		munit_assert_int64(second->timestamp, ==, 250);
		munit_assert_ptr_not_null(second->next);

		ChipEvent *sentinel = second->next;
		munit_assert_uint64(sentinel->chip_mask, ==, 0);
		munit_assert_int64(sentinel->timestamp, ==, INT64_MAX);
		munit_assert_ptr_null(sentinel->next);
	}

	simulator_schedule_event(simulator, 2, 200);
	munit_assert_ptr_not_null(simulator->event_schedule[0]);
	{
		ChipEvent *first = simulator->event_schedule[0];
		munit_assert_uint64(first->chip_mask, ==, 0b0010);
		munit_assert_int64(first->timestamp, ==, 150);
		munit_assert_ptr_not_null(first->next);

		ChipEvent *second = first->next;
		munit_assert_uint64(second->chip_mask, ==, 0b0100);
		munit_assert_int64(second->timestamp, ==, 200);
		munit_assert_ptr_not_null(second->next);

		ChipEvent *third = second->next;
		munit_assert_uint64(third->chip_mask, ==, 0b1000);
		munit_assert_int64(third->timestamp, ==, 250);
		munit_assert_ptr_not_null(third->next);

		ChipEvent *sentinel = third->next;
		munit_assert_uint64(sentinel->chip_mask, ==, 0);
		munit_assert_int64(sentinel->timestamp, ==, INT64_MAX);
		munit_assert_ptr_null(sentinel->next);
	}

	simulator_schedule_event(simulator, 0, 200);
	munit_assert_ptr_not_null(simulator->event_schedule[0]);
	{
		ChipEvent *first = simulator->event_schedule[0];
		munit_assert_uint64(first->chip_mask, ==, 0b0010);
		munit_assert_int64(first->timestamp, ==, 150);
		munit_assert_ptr_not_null(first->next);

		ChipEvent *second = first->next;
		munit_assert_uint64(second->chip_mask, ==, 0b0101);
		munit_assert_int64(second->timestamp, ==, 200);
		munit_assert_ptr_not_null(second->next);

		ChipEvent *third = second->next;
		munit_assert_uint64(third->chip_mask, ==, 0b1000);
		munit_assert_int64(third->timestamp, ==, 250);
		munit_assert_ptr_not_null(third->next);

		ChipEvent *sentinel = third->next;
		munit_assert_uint64(sentinel->chip_mask, ==, 0);
		munit_assert_int64(sentinel->timestamp, ==, INT64_MAX);
		munit_assert_ptr_null(sentinel->next);
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

	munit_assert_int64(simulator_next_scheduled_event_timestamp(simulator), ==, 150);
	munit_assert_uint64(simulator_pop_next_scheduled_event(simulator, 100), ==, 0);
	munit_assert_uint64(simulator_pop_next_scheduled_event(simulator, 150), ==, 0b0010);
	munit_assert_uint64(simulator_pop_next_scheduled_event(simulator, 150), ==, 0);

	munit_assert_int64(simulator_next_scheduled_event_timestamp(simulator), ==, 200);
	munit_assert_uint64(simulator_pop_next_scheduled_event(simulator, 200), ==, 0b0100);
	munit_assert_uint64(simulator_pop_next_scheduled_event(simulator, 200), ==, 0);

	munit_assert_int64(simulator_next_scheduled_event_timestamp(simulator), ==, 250);
	munit_assert_uint64(simulator_pop_next_scheduled_event(simulator, 250), ==, 0b1000);
	munit_assert_uint64(simulator_pop_next_scheduled_event(simulator, 250), ==, 0);

	munit_assert_int64(simulator_next_scheduled_event_timestamp(simulator), ==, INT64_MAX);

	simulator_destroy(simulator);
	return MUNIT_OK;
}

MunitTest simulator_tests[] = {
	{ "/schedule_event", test_schedule_event, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/pop_event", test_pop_event, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
