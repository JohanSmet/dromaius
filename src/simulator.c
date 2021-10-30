// simulator.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Core simulation functions

#include "simulator.h"
#include "chip.h"
#include "signal_line.h"
#include "sys/threads.h"

//#define DMS_LOG_TRACE
#define LOG_SIMULATOR	PUBLIC(sim)
#include "log.h"

#include <stb/stb_ds.h>
#include <assert.h>
#include <inttypes.h>
#include <stdatomic.h>

///////////////////////////////////////////////////////////////////////////////
//
// types
//

#define NUM_SIMULATOR_THREADS	2					// TODO: hardcoded for now, change to be dynamic
#define MAX_SCHEDULED_EVENTS	16					// TODO: should be set by device

#ifndef DMS_NO_THREADING

static const uint64_t THREAD_CHIP_MASK[NUM_SIMULATOR_THREADS] = {
	0x5555555555555555,
	0xAAAAAAAAAAAAAAAA
};

typedef struct WorkerInfo {
	thread_t					thread;
	size_t						thread_id;
	struct Simulator_private *	sim;
}   WorkerInfo;
#endif // DMS_NO_THREADING

typedef struct Simulator_private {
	Simulator				public;

	Chip **					chips;
	uint64_t				dirty_chips;

	ChipEvent *				event_buffer;
	ChipEvent **			event_free_pool;				// re-use pool

#ifndef DMS_NO_THREADING
	mutex_t					mtx_process;
	cond_t					cnd_work_available;
	cond_t					cnd_work_all_done;
	size_t					workers_to_start;
	size_t					workers_to_finish;

	WorkerInfo				workers[NUM_SIMULATOR_THREADS];
#endif

} Simulator_private;

#define PRIVATE(sim)	((Simulator_private *) (sim))
#define PUBLIC(sim)		(&(sim)->public)


///////////////////////////////////////////////////////////////////////////////
//
// helper functions
//

static inline void sim_process_sequential(Simulator_private *sim, uint64_t dirty_chips) {

	while (dirty_chips > 0) {

		// find the lowest set bit
		int32_t chip_id = bit_lowest_set(dirty_chips);

		// process
		Chip *chip = sim->chips[chip_id];
		chip->process(chip);

		if (chip->schedule_timestamp > 0) {
			simulator_schedule_event(PUBLIC(sim), chip->id, chip->schedule_timestamp);
			chip->schedule_timestamp = 0;
		}

		// clear the lowest bit
		dirty_chips &= ~(1ull << chip_id);
	}
}

#ifndef DMS_NO_THREADING

static int sim_worker_thread(WorkerInfo *worker) {
	assert(worker);

	Simulator_private *sim = worker->sim;

	LOG_TRACE("SimWorker [%d]: worker thread starting", worker->thread_id);

	// set signal queue indices for this thread
	signal_write_queue_id = worker->thread_id + 1;
	signal_highz_queue_id = worker->thread_id;

	while (true) {

		// wait for work
		mutex_lock(&sim->mtx_process);
		while (sim->workers_to_start == 0) {
			LOG_TRACE("SimWorker [%d]: waiting for work", worker->thread_id);
			cond_wait(&sim->cnd_work_available, &sim->mtx_process);
		}
		--sim->workers_to_start;
		uint64_t chip_mask = THREAD_CHIP_MASK[sim->workers_to_start];
		LOG_TRACE("SimWorker [%d]: work available (%d workers to finish / %d workers to start)",
					worker->thread_id, sim->workers_to_finish, sim->workers_to_start);
		mutex_unlock(&sim->mtx_process);

		// execute the work
		LOG_TRACE("SimWorker [%d]: dirty chips = 0x%016" PRIX64 " - mask = 0x%016" PRIX64 " => running chips 0x%016" PRIX64 "",
					worker->thread_id, sim->dirty_chips, chip_mask, sim->dirty_chips & chip_mask);
		sim_process_sequential(sim, sim->dirty_chips & chip_mask);

		// mark work as done
		mutex_lock(&sim->mtx_process);
		bool all_done = (--sim->workers_to_finish) == 0;
		LOG_TRACE("SimWorker [%d]: work done (%d workers to finish / %d workers to start)",
					worker->thread_id, sim->workers_to_finish, sim->workers_to_start);
		mutex_unlock(&sim->mtx_process);

		if (all_done) {
			LOG_TRACE("SimWorker [%d]: all work done", worker->thread_id);
			cnd_signal(&sim->cnd_work_all_done);
		}
	}

	return 0;
}

static inline void sim_process_concurrent(Simulator_private *sim) {

	// wake up the worker threads
	mutex_lock(&sim->mtx_process);
	LOG_TRACE("Simulator [manager]: work available (0x%016" PRIX64 ")", sim->dirty_chips);
	sim->workers_to_start = NUM_SIMULATOR_THREADS;
	sim->workers_to_finish = NUM_SIMULATOR_THREADS;
	cond_signal_all(&sim->cnd_work_available);

	// wait until the work is done
	while (sim->workers_to_finish > 0) {
		cond_wait(&sim->cnd_work_all_done, &sim->mtx_process);
	}
	mutex_unlock(&sim->mtx_process);
}

#endif // DMS_NO_THREADING

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Simulator *simulator_create(int64_t tick_duration_ps) {
	Simulator_private *priv_sim = (Simulator_private *) calloc(1, sizeof(Simulator_private));

	// TODO: max number of concurrent signals should be given by the caller
	PUBLIC(priv_sim)->signal_pool = signal_pool_create(NUM_SIMULATOR_THREADS, 256);
	PUBLIC(priv_sim)->tick_duration_ps = tick_duration_ps;

	// allocate scheduled events
	PUBLIC(priv_sim)->event_schedule = calloc(NUM_SIMULATOR_THREADS, sizeof(ChipEvent *));
	priv_sim->event_free_pool = calloc(NUM_SIMULATOR_THREADS, sizeof(ChipEvent *));
	priv_sim->event_buffer = (ChipEvent *) malloc((MAX_SCHEDULED_EVENTS + 1) * NUM_SIMULATOR_THREADS * sizeof(ChipEvent));

	// >> sentinels for the schedule event linked lists
	ChipEvent *event_ptr = priv_sim->event_buffer;

	for (size_t thread_id = 0; thread_id < NUM_SIMULATOR_THREADS; ++thread_id) {
		event_ptr->chip_mask = 0;
		event_ptr->timestamp = INT64_MAX;
		event_ptr->next		 = NULL;

		PUBLIC(priv_sim)->event_schedule[thread_id] = event_ptr++;
	}

	// >> prime free pool
	for (size_t thread_id = 0; thread_id < NUM_SIMULATOR_THREADS; ++thread_id) {
		for (size_t idx = 0; idx < MAX_SCHEDULED_EVENTS; ++idx) {
			event_ptr->chip_mask = 0;
			event_ptr->timestamp = 0;
			event_ptr->next = priv_sim->event_free_pool[thread_id];
			priv_sim->event_free_pool[thread_id] = event_ptr++;
		}
	}

#ifndef DMS_NO_THREADING
	// threading
	mutex_init_plain(&priv_sim->mtx_process);
	cond_init(&priv_sim->cnd_work_available);
	cond_init(&priv_sim->cnd_work_all_done);
	priv_sim->workers_to_start = 0;
	priv_sim->workers_to_finish = 0;

	for (size_t idx = 0; idx < NUM_SIMULATOR_THREADS; ++idx) {
		priv_sim->workers[idx].sim = priv_sim;
		priv_sim->workers[idx].thread_id = idx;

		thread_create_joinable(&priv_sim->workers[idx].thread,
							   (thread_func_t) sim_worker_thread, &priv_sim->workers[idx]);
	}


#endif // DMS_NO_THREADING

	return &priv_sim->public;
}

void simulator_destroy(Simulator *sim) {
	assert(sim);

	for (int32_t id = 0; id < arrlen(PRIVATE(sim)->chips); ++id) {
		PRIVATE(sim)->chips[id]->destroy(PRIVATE(sim)->chips[id]);
	}

	arrfree(PRIVATE(sim)->chips);

	signal_pool_destroy(sim->signal_pool);

	free(PRIVATE(sim)->event_buffer);
	free(PRIVATE(sim)->event_free_pool);
	free(sim->event_schedule);

	free(PRIVATE(sim));
}

Chip *simulator_register_chip(Simulator *sim, Chip *chip, const char *name) {
	assert(sim);
	assert(chip);

	chip->id = (int32_t) arrlenu(PRIVATE(sim)->chips);
	chip->name = name;
	chip->simulator = sim;
	arrpush(PRIVATE(sim)->chips, chip);
	PRIVATE(sim)->dirty_chips |= 1ull << chip->id;
	return chip;
}

Chip *simulator_chip_by_name(Simulator *sim, const char *name) {
	assert(sim);
	assert(name);

	for (int32_t id = 0; id < arrlen(PRIVATE(sim)->chips); ++id) {
		if (strcmp(PRIVATE(sim)->chips[id]->name, name) == 0) {
			return PRIVATE(sim)->chips[id];
		}
	}

	return NULL;
}

const char *simulator_chip_name(Simulator *sim, int32_t chip_id) {
	assert(sim);

	if (chip_id >= 0 && chip_id < arrlen(PRIVATE(sim)->chips)) {
		return PRIVATE(sim)->chips[chip_id]->name;
	} else {
		return NULL;
	}
}

void simulator_device_complete(Simulator *sim) {
	assert(sim);

	// register dependencies
	for (int32_t id = 0; id < arrlen(PRIVATE(sim)->chips); ++id) {
		PRIVATE(sim)->chips[id]->register_dependencies(PRIVATE(sim)->chips[id]);
	}
}

void simulator_simulate_timestep(Simulator *sim) {
	assert(sim);

	// advance to next timestamp
	if (PRIVATE(sim)->dirty_chips > 0) {
		++sim->current_tick;
	} else {
		// advance to next scheduled event if no chips to be processed
		sim->current_tick = simulator_next_scheduled_event_timestamp(sim);
	}

	// handle scheduled events for the current timestamp
	PRIVATE(sim)->dirty_chips |= simulator_pop_next_scheduled_event(sim, sim->current_tick);

	// process all chips that have a dependency on signal that was changed in the last timestep or have a scheduled wakeup
#ifdef DMS_NO_THREADING
	sim_process_sequential(PRIVATE(sim), PRIVATE(sim)->dirty_chips);
#else
	sim_process_concurrent(PRIVATE(sim));
#endif

	// process any chips that wrote to a signal of which the active writers changed
	//	(TODO: shouldn't really call processs, only outputting previous signals should be enough/safer)
	uint64_t rerun_chips = signal_pool_process_high_impedance(sim->signal_pool);
	sim_process_sequential(PRIVATE(sim), rerun_chips);

	// determine changed signals and dirty chips for next simulation step
	PRIVATE(sim)->dirty_chips = signal_pool_cycle(sim->signal_pool);
}

void simulator_schedule_event(Simulator *sim, int32_t chip_id, int64_t timestamp) {
	assert(sim);

	const size_t thread_id = signal_highz_queue_id;

	// find the insertion spot
	ChipEvent *next  = sim->event_schedule[thread_id];
	ChipEvent **prev = &sim->event_schedule[thread_id];

	while (next && next->timestamp <= timestamp) {
		if (next->timestamp == timestamp) {
			next->chip_mask |= 1ull << chip_id;
			return;
		}

		prev = &next->next;
		next = next->next;
	}

	// get an allocated event structure from the free pool
	ChipEvent *event = PRIVATE(sim)->event_free_pool[thread_id];
	assert(event != NULL);
	PRIVATE(sim)->event_free_pool[thread_id] = event->next;

	event->chip_mask = 1ull << chip_id;
	event->timestamp = timestamp;
	event->next = next;

	*prev = event;
}

int64_t simulator_next_scheduled_event_timestamp(Simulator *sim) {
	int64_t result = INT64_MAX;

	for (size_t thread_id = 0; thread_id < NUM_SIMULATOR_THREADS; ++thread_id) {
		if (sim->event_schedule[thread_id]->timestamp < result) {
			result = sim->event_schedule[thread_id]->timestamp;
		}
	}

	return result;
}

uint64_t simulator_pop_next_scheduled_event(Simulator *sim, int64_t timestamp) {
	assert(sim);
	assert(timestamp < INT64_MAX);

	uint64_t result = 0;

	for (size_t thread_id = 0; thread_id < NUM_SIMULATOR_THREADS; ++thread_id) {
		ChipEvent *event = sim->event_schedule[thread_id];
		if (event->timestamp == timestamp) {
			result |= event->chip_mask;

			// remove from schedule
			sim->event_schedule[thread_id] = event->next;

			// add to re-use pool
			event->next = PRIVATE(sim)->event_free_pool[thread_id];
			PRIVATE(sim)->event_free_pool[thread_id] = event;
		}
	}

	return result;
}

