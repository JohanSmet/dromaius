// simulator.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Core simulation functions

#include "simulator.h"
#include "chip.h"
#include "signal_line.h"

#include "sys/threads.h"

#include <stb/stb_ds.h>
#include <assert.h>

//#define DMS_LOG_TRACE
#include "log.h"

#define LOG_SIMULATOR		PUBLIC(sim)

///////////////////////////////////////////////////////////////////////////////
//
// types
//

#ifndef DMS_NO_THREADING
	typedef struct WorkerInfo {
		thread_t				  thread;
		size_t					  thread_id;
		struct Simulator_private *sim;
	} WorkerInfo;
#endif

typedef struct Simulator_private {
	Simulator				public;

	Chip **					chips;
	int32_t *				dirty_chips;
	bool *					chip_is_dirty;

	ChipEvent *				event_pool;				// re-use pool

#ifndef DMS_NO_THREADING
	size_t					jobs_to_start;
	size_t					jobs_to_finish;
	mutex_t					mtx_process;
	cond_t					cnd_work_available;
	cond_t					cnd_work_done;
	WorkerInfo				thrd_workers[1];
#endif

} Simulator_private;

#define PRIVATE(sim)	((Simulator_private *) (sim))
#define PUBLIC(sim)		(&(sim)->public)

///////////////////////////////////////////////////////////////////////////////
//
// helper functions
//

static inline int64_t simulator_next_scheduled_event_timestamp(Simulator_private *sim) {
	return PUBLIC(sim)->event_schedule->timestamp;
}

static inline void sim_handle_event_schedule(Simulator_private *sim) {
	int32_t chip_id = simulator_pop_scheduled_event(PUBLIC(sim), PUBLIC(sim)->current_tick);

	while (chip_id >= 0) {
		if (!sim->chip_is_dirty[chip_id]) {
			arrpush(sim->dirty_chips, chip_id);
			sim->chip_is_dirty[chip_id] = true;
		}

		chip_id = simulator_pop_scheduled_event(PUBLIC(sim), PUBLIC(sim)->current_tick);
	}
}

static inline void sim_process_sequential(Simulator_private *sim) {
	for (size_t idx = 0; idx < arrlenu(sim->dirty_chips); ++idx) {
		Chip *chip = sim->chips[sim->dirty_chips[idx]];
		chip->process(chip);

		if (chip->schedule_timestamp > 0) {
			simulator_schedule_event(PUBLIC(sim), chip->id, chip->schedule_timestamp);
			chip->schedule_timestamp = 0;
		}
	}
}

#ifndef DMS_NO_THREADING

static int sim_worker_thread(WorkerInfo *worker) {
	Simulator_private *sim = worker->sim;
	LOG_TRACE("SimWorker [%d]: worker thread starting", worker->thread_id);

	int32_t chip_id = -1;
	signal_pool_queue_id = worker->thread_id;

	while (true) {

		// wait for a new job
		mutex_lock(&sim->mtx_process);
			while (sim->jobs_to_start == 0) {
				LOG_TRACE("SimWorker [%d]: wait for work (work=%d/%d)", worker->thread_id, sim->jobs_to_start, sim->jobs_to_finish);
				cond_wait(&sim->cnd_work_available, &sim->mtx_process);
			}

			LOG_TRACE("SimWorker [%d]: have work (chips=%d/%d)", worker->thread_id, sim->jobs_to_start, sim->jobs_to_finish);
			chip_id = arrpop(sim->dirty_chips);
			sim->jobs_to_start -= 1;
			LOG_TRACE("SimWorker [%d]: process chip %d", worker->thread_id, chip_id);
		mutex_unlock(&sim->mtx_process);

		// process the chip
		Chip *chip = sim->chips[chip_id];
		chip->process(chip);

		// scheduling
		if (chip->schedule_timestamp > 0) {
			mutex_lock(&sim->mtx_process);
			simulator_schedule_event(PUBLIC(sim), chip->id, chip->schedule_timestamp);
			mutex_unlock(&sim->mtx_process);
			chip->schedule_timestamp = 0;
		}

		// mark job as done
		mutex_lock(&sim->mtx_process);
			LOG_TRACE("SimWorker [%d]: process chip %d done (%d/%d left)", worker->thread_id, chip_id, sim->jobs_to_start, sim->jobs_to_finish);
			sim->jobs_to_finish -= 1;
			bool all_done = sim->jobs_to_finish == 0;
		mutex_unlock(&sim->mtx_process);

		if (all_done) {
			cnd_signal(&sim->cnd_work_done);
		}
	}

	return 0;
}

static inline void sim_process_threadpool(Simulator_private *sim) {

	if (arrlen(sim->dirty_chips) <= 2) {
		return sim_process_sequential(sim);
	}

	// wake up the worker threads
	mutex_lock(&sim->mtx_process);
	sim->jobs_to_start = arrlenu(sim->dirty_chips);
	sim->jobs_to_finish = sim->jobs_to_start;
	LOG_TRACE("Simulator [manager]: wake up worker threads (todo=%d/%d)", sim->jobs_to_start, sim->jobs_to_finish);
	cond_signal_all(&sim->cnd_work_available);

	// wait until all work is done
	while (sim->jobs_to_finish > 0) {
		LOG_TRACE("Simulator [manager]: wait for worker threads (todo=%d/%d)", sim->jobs_to_start, sim->jobs_to_finish);
		cond_wait(&sim->cnd_work_done, &sim->mtx_process);
	}
	LOG_TRACE("Simulator [manager]: work done (%d)", 0);
	mutex_unlock(&sim->mtx_process);
}

#endif // DMS_NO_THREADING

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Simulator *simulator_create(int64_t tick_duration_ps) {
	Simulator_private *priv_sim = (Simulator_private *) calloc(1, sizeof(Simulator_private));

	PUBLIC(priv_sim)->signal_pool = signal_pool_create();
	PUBLIC(priv_sim)->tick_duration_ps = tick_duration_ps;

#ifndef DMS_NO_THREADING
	size_t num_workers = sizeof(priv_sim->thrd_workers) / sizeof(priv_sim->thrd_workers[0]);

	signal_pool_thread_count(PUBLIC(priv_sim)->signal_pool, num_workers);

	mutex_init_plain(&priv_sim->mtx_process);
	cond_init(&priv_sim->cnd_work_available);
	cond_init(&priv_sim->cnd_work_done);
	for (size_t i = 0; i < num_workers; ++i) {
		priv_sim->thrd_workers[i].thread_id = i;
		priv_sim->thrd_workers[i].sim = priv_sim;
		thread_create_joinable(&priv_sim->thrd_workers[i].thread, (thread_func_t) sim_worker_thread, &priv_sim->thrd_workers[i]);
	}
#endif

	return &priv_sim->public;
}

void simulator_destroy(Simulator *sim) {
	assert(sim);

	for (int32_t id = 0; id < arrlen(PRIVATE(sim)->chips); ++id) {
		PRIVATE(sim)->chips[id]->destroy(PRIVATE(sim)->chips[id]);
	}

	arrfree(PRIVATE(sim)->chips);
	arrfree(PRIVATE(sim)->chip_is_dirty);
	arrfree(PRIVATE(sim)->dirty_chips);

	signal_pool_destroy(sim->signal_pool);
	free(PRIVATE(sim));
}

Chip *simulator_register_chip(Simulator *sim, Chip *chip, const char *name) {
	assert(sim);
	assert(chip);

	chip->id = (int32_t) arrlenu(PRIVATE(sim)->chips);
	chip->name = name;
	chip->simulator = sim;
	arrpush(PRIVATE(sim)->chips, chip);
	arrpush(PRIVATE(sim)->chip_is_dirty, true);
	arrpush(PRIVATE(sim)->dirty_chips, chip->id);
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
	if (arrlenu(PRIVATE(sim)->dirty_chips)) {
		++sim->current_tick;
	} else {
		// advance to next scheduled event if no chips to be processed
		sim->current_tick = simulator_next_scheduled_event_timestamp(PRIVATE(sim));
	}

	// handle scheduled events for the current timestamp
	sim_handle_event_schedule(PRIVATE(sim));

	// process all chips that have a dependency on signal that was changed in the last timestep or have a scheduled wakeup
	#ifdef DMS_NO_THREADING
		sim_process_sequential(PRIVATE(sim));
	#else
		sim_process_threadpool(PRIVATE(sim));
	#endif

	// determine changed signals and dirty chips for next simulation step
	memset(PRIVATE(sim)->chip_is_dirty, false, arrlenu(PRIVATE(sim)->chip_is_dirty));
	if (PRIVATE(sim)->dirty_chips) {
		stbds_header(PRIVATE(sim)->dirty_chips)->length = 0;
	}

	signal_pool_cycle_dirty_flags(sim->signal_pool, sim->current_tick, PRIVATE(sim)->chip_is_dirty, &PRIVATE(sim)->dirty_chips);
}

void simulator_schedule_event(Simulator *sim, int32_t chip_id, int64_t timestamp) {
	assert(sim);

	// find the insertion spot
	ChipEvent *next = sim->event_schedule;
	ChipEvent **prev = &sim->event_schedule;

	while (next && next->timestamp < timestamp) {
		if (next->timestamp == timestamp && next->chip_id == chip_id) {
			return;				// no duplicates
		}

		prev = &next->next;
		next = next->next;
	}

	// try to get an allocated event structure from the free pool, or allocate a new one
	ChipEvent *event = PRIVATE(sim)->event_pool;

	if (event) {
		PRIVATE(sim)->event_pool = event->next;
		event->next = NULL;
	} else {
		event = (ChipEvent *) malloc(sizeof(ChipEvent));
	}

	event->chip_id = chip_id;
	event->timestamp = timestamp;
	event->next = NULL;

	// insert the event
	*prev = event;
	event->next = next;
}

int32_t simulator_pop_scheduled_event(Simulator *sim, int64_t timestamp) {
	assert(sim);

	if (!sim->event_schedule || sim->event_schedule->timestamp != timestamp) {
		return -1;
	}

	ChipEvent *event = sim->event_schedule;
	int32_t result = event->chip_id;

	// remove from schedule
	sim->event_schedule = event->next;

	// add to reuse pool
	event->next = PRIVATE(sim)->event_pool;
	PRIVATE(sim)->event_pool = event;

	return result;
}

