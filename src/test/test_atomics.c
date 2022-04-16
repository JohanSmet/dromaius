// test/test_utils.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"

#include "sys/atomics.h"
#include "sys/threads.h"

////////////////////////////////////////////////////////////////////////////////
//
// test_atomic_flag
//

struct AtomicFlagData {
	flag_t		lock;
	volatile uint32_t	value;
};

static int thread_test_atomic_flag(struct AtomicFlagData *data) {

	for (size_t i = 0; i < 10000; ++i) {
		flag_acquire_lock(&data->lock);
		data->value += 1;
		flag_release_lock(&data->lock);
	}
	return 0;
}

static MunitResult test_atomic_flag(const MunitParameter params[], void* user_data_or_fixture) {

	struct AtomicFlagData test_data = {FLAG_INIT, 0};

	thread_t threads[10] = {0};

	// create the threads
	for (size_t i = 0; i < 10; ++i) {
		thread_create_joinable(&threads[i], (thread_func_t) thread_test_atomic_flag, &test_data);
	}

	// wait for threads to finish
	for (size_t i = 0; i < 10; ++i) {
		int thread_result;
		thread_join(threads[i], &thread_result);
	}

	munit_assert_uint32(test_data.value, ==, 100000);

    return MUNIT_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
// test_atomic_exchange_load
//

#define EXCHANGE_TEST_COUNT		128
#define EXCHANGE_QUEUE_SIZE		32

struct ExchangeData {
	atomic_uint32_t		head;		// next index to write to
	atomic_uint32_t		tail;		// first index to read from
	uint32_t			queue[EXCHANGE_QUEUE_SIZE];
};

static int thread_test_exchange_producer(struct ExchangeData *data) {

	for (size_t count = EXCHANGE_TEST_COUNT; count > 0; --count) {
		// don't let next_in index 'catch up' with first_out index
		while (((data->head + 1) % EXCHANGE_QUEUE_SIZE) == atomic_load_uint32(&data->tail)) {
			// spin-lock
		}

		// update data
		munit_assert_uint32(data->queue[data->head], ==, 0);
		data->queue[data->head] = 1;

		// move pointer along
		atomic_exchange_uint32(&data->head, (data->head + 1) % EXCHANGE_QUEUE_SIZE);
	}

	return MUNIT_OK;
}

static int thread_test_exchange_consumer(struct ExchangeData *data) {

	for (size_t count = EXCHANGE_TEST_COUNT; count > 0; --count) {

		// wait until there is work to do
		while (data->tail == atomic_load_uint32(&data->head)) {
			// spinlock
		}

		// update data
		munit_assert_uint32(data->queue[data->tail], ==, 1);
		data->queue[data->tail] = 0;

		atomic_exchange_uint32(&data->tail, (data->tail + 1) % EXCHANGE_QUEUE_SIZE);
	}

	return MUNIT_OK;
}

static MunitResult test_atomic_exchange_load(const MunitParameter params[], void* user_data_or_fixture) {

	struct ExchangeData test_data = {0};

	thread_t consumer = 0;
	thread_t producer = 0;

	// create threads
	thread_create_joinable(&consumer, (thread_func_t) thread_test_exchange_consumer, &test_data);
	thread_create_joinable(&producer, (thread_func_t) thread_test_exchange_producer, &test_data);

	// wait for threads to end
	int thread_result;

	thread_join(producer, &thread_result);
	if (thread_result != MUNIT_OK) {
		return thread_result;
	}

	thread_join(consumer, &thread_result);
	if (thread_result != MUNIT_OK) {
		return thread_result;
	}

    return MUNIT_OK;
}

MunitTest atomics_tests[] = {
    { "/atomic_flag", test_atomic_flag, NULL, NULL,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/exchange_load", test_atomic_exchange_load, NULL, NULL,  MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
