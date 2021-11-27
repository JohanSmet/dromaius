// test/test_device.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "simulator.h"

#include "chip_dummy.h"

static void *simulator_setup(const MunitParameter params[], void *user_data) {
	Simulator *simulator = simulator_create(NS_TO_PS(100));
	return simulator;
}

static void simulator_teardown(void *fixture) {
	simulator_destroy((Simulator *) fixture);
}

MunitResult test_schedule_event(const MunitParameter params[], void *user_data_or_fixture) {

	Simulator *simulator = (Simulator *) user_data_or_fixture;

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

	return MUNIT_OK;
}

MunitResult test_pop_event(const MunitParameter params[], void *user_data_or_fixture) {
	Simulator *simulator = (Simulator *) user_data_or_fixture;
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

	return MUNIT_OK;
}

MunitResult test_signal_writers(const MunitParameter params[], void *user_data_or_fixture) {

	Simulator *simulator = (Simulator *) user_data_or_fixture;
	munit_assert_ptr_not_null(simulator);

	// setup
	Signal s0 = signal_create(simulator->signal_pool);
	Signal s1 = signal_create(simulator->signal_pool);

	ChipDummy *c1 = chip_dummy_create(simulator, (Signal[CHIP_DUMMY_PIN_COUNT]) {
										[CHIP_DUMMY_O0] = s0,
										[CHIP_DUMMY_O1] = s1
	});
	ChipDummy *c2 = chip_dummy_create(simulator, (Signal[CHIP_DUMMY_PIN_COUNT]) {
										[CHIP_DUMMY_O0] = s0,
										[CHIP_DUMMY_O1] = s1
	});
	simulator_register_chip(simulator, (Chip *) c1, "C1");
	simulator_register_chip(simulator, (Chip *) c2, "C2");
	simulator_device_complete(simulator);

	munit_assert_int32(c1->id, ==, 0);
	munit_assert_int32(c2->id, ==, 1);

	// no writers
	signal_pool_cycle(simulator->signal_pool);
	munit_assert_uint64(simulator_signal_writers(simulator, s0), ==, 0ull);
	munit_assert_uint64(simulator_signal_writers(simulator, s1), ==, 0ull);

	// one writer
	signal_write(simulator->signal_pool, c1->signals[CHIP_DUMMY_O0], false);
	signal_pool_cycle(simulator->signal_pool);
	munit_assert_uint64(simulator_signal_writers(simulator, s0), ==, 0x1ull);
	munit_assert_uint64(simulator_signal_writers(simulator, s1), ==, 0x0ull);

	// two writers
	signal_write(simulator->signal_pool, c2->signals[CHIP_DUMMY_O0], false);
	signal_pool_cycle(simulator->signal_pool);
	munit_assert_uint64(simulator_signal_writers(simulator, s0), ==, 0x3ull);
	munit_assert_uint64(simulator_signal_writers(simulator, s1), ==, 0x0ull);

	// back to one writer
	signal_clear_writer(simulator->signal_pool, c1->signals[CHIP_DUMMY_O0]);
	signal_pool_cycle(simulator->signal_pool);
	munit_assert_uint64(simulator_signal_writers(simulator, s0), ==, 0x2ull);
	munit_assert_uint64(simulator_signal_writers(simulator, s1), ==, 0x0ull);

	// write another signal
	signal_write(simulator->signal_pool, c1->signals[CHIP_DUMMY_O1], false);
	signal_pool_cycle(simulator->signal_pool);
	munit_assert_uint64(simulator_signal_writers(simulator, s0), ==, 0x2ull);
	munit_assert_uint64(simulator_signal_writers(simulator, s1), ==, 0x1ull);

	// stop writing
	signal_clear_writer(simulator->signal_pool, c2->signals[CHIP_DUMMY_O0]);
	signal_clear_writer(simulator->signal_pool, c1->signals[CHIP_DUMMY_O1]);
	signal_pool_cycle(simulator->signal_pool);
	munit_assert_uint64(simulator_signal_writers(simulator, s0), ==, 0ull);
	munit_assert_uint64(simulator_signal_writers(simulator, s1), ==, 0ull);

	return MUNIT_OK;
}

MunitTest simulator_tests[] = {
	{ "/schedule_event", test_schedule_event, simulator_setup, simulator_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/pop_event", test_pop_event, simulator_setup, simulator_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/signal_writers", test_signal_writers, simulator_setup, simulator_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
