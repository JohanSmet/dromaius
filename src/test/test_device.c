// test/test_device.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "device.h"

#include "stb/stb_ds.h"

MunitResult test_schedule_event(const MunitParameter params[], void *user_data_or_fixture) {

	Device *device = (Device *) calloc(1, sizeof(Device));

	munit_assert_ptr_not_null(device);
	munit_assert_ptr_null(device->event_pool);
	munit_assert_ptr_null(device->event_schedule);

	device_schedule_event(device, 3, 250);
	munit_assert_ptr_null(device->event_pool);
	munit_assert_ptr_not_null(device->event_schedule);
	munit_assert_uint32(device->event_schedule->chip_id, ==, 3);
	munit_assert_uint64(device->event_schedule->timestamp, ==, 250);
	munit_assert_ptr_null(device->event_schedule->next);

	device_schedule_event(device, 1, 150);
	munit_assert_ptr_null(device->event_pool);
	munit_assert_ptr_not_null(device->event_schedule);
	{
		ChipEvent *first = device->event_schedule;
		munit_assert_uint32(first->chip_id, ==, 1);
		munit_assert_uint64(first->timestamp, ==, 150);
		munit_assert_ptr_not_null(first->next);

		ChipEvent *second = first->next;
		munit_assert_uint32(second->chip_id, ==, 3);
		munit_assert_uint64(second->timestamp, ==, 250);
		munit_assert_ptr_null(second->next);
	}

	device_schedule_event(device, 2, 200);
	munit_assert_ptr_null(device->event_pool);
	munit_assert_ptr_not_null(device->event_schedule);
	{
		ChipEvent *first = device->event_schedule;
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

	free(device);
	return MUNIT_OK;
}

MunitResult test_pop_event(const MunitParameter params[], void *user_data_or_fixture) {
	Device *device = (Device *) calloc(1, sizeof(Device));
	munit_assert_ptr_not_null(device);

	device_schedule_event(device, 3, 250);
	device_schedule_event(device, 1, 150);
	device_schedule_event(device, 2, 200);

	munit_assert_int64(device_next_scheduled_event_timestamp(device), ==, 150);

	munit_assert_int32(device_pop_scheduled_event(device, 100), ==, -1);

	munit_assert_int32(device_pop_scheduled_event(device, 150), ==, 1);
	munit_assert_ptr_not_null(device->event_pool);
	munit_assert_int32(device_pop_scheduled_event(device, 150), ==, -1);
	munit_assert_int64(device_next_scheduled_event_timestamp(device), ==, 200);

	munit_assert_int32(device_pop_scheduled_event(device, 200), ==, 2);
	munit_assert_int32(device_pop_scheduled_event(device, 200), ==, -1);
	munit_assert_int64(device_next_scheduled_event_timestamp(device), ==, 250);

	munit_assert_int32(device_pop_scheduled_event(device, 250), ==, 3);
	munit_assert_int32(device_pop_scheduled_event(device, 250), ==, -1);
	munit_assert_ptr_null(device->event_schedule);

	free(device);
	return MUNIT_OK;
}

MunitTest device_tests[] = {
	{ "/schedule_event", test_schedule_event, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/pop_event", test_pop_event, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
