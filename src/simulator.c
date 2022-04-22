// simulator.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Core simulation functions

#include "simulator.h"
#include "chip.h"
#include "crt.h"
#include "signal_line.h"
#include "signal_history.h"

#include <stb/stb_ds.h>
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////
//
// types
//

typedef struct Simulator_private {
	Simulator				public;

	Chip **					chips;
	uint64_t				dirty_chips;
	uint64_t				layer_chips[SIGNAL_LAYERS];

	ChipEvent *				event_pool;				// re-use pool
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
		sim->dirty_chips |= 1ull << chip_id;
		chip_id = simulator_pop_scheduled_event(PUBLIC(sim), PUBLIC(sim)->current_tick);
	}
}

static inline void sim_process_sequential(Simulator_private *sim, uint64_t dirty_chips) {

	while (dirty_chips > 0) {

		// find the lowest set bit
		int32_t chip_id = bit_lowest_set(dirty_chips);
		dirty_chips &= dirty_chips - 1;

		// process
		Chip *chip = sim->chips[chip_id];
		chip->process(chip);

		if (chip->schedule_timestamp > 0) {
			simulator_schedule_event(PUBLIC(sim), chip->id, chip->schedule_timestamp);
			chip->schedule_timestamp = 0;
		}
	}
}

static void simulator_free_event_list(ChipEvent *event) {
	while (event) {
		ChipEvent *next = event->next;
		dms_free(event);
		event = next;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Simulator *simulator_create(int64_t tick_duration_ps) {
	Simulator_private *priv_sim = (Simulator_private *) dms_calloc(1, sizeof(Simulator_private));

	PUBLIC(priv_sim)->signal_pool = signal_pool_create();
	PUBLIC(priv_sim)->tick_duration_ps = tick_duration_ps;

	return &priv_sim->public;
}

void simulator_destroy(Simulator *sim) {
	assert(sim);

	for (int32_t id = 0; id < arrlen(PRIVATE(sim)->chips); ++id) {
		PRIVATE(sim)->chips[id]->destroy(PRIVATE(sim)->chips[id]);
	}

	simulator_free_event_list(sim->event_schedule);
	simulator_free_event_list(PRIVATE(sim)->event_pool);

	arrfree(PRIVATE(sim)->chips);

	if (sim->signal_history) {
		signal_history_process_stop(sim->signal_history);
		signal_history_destroy(sim->signal_history);
	}

	signal_pool_destroy(sim->signal_pool);
	dms_free(PRIVATE(sim));
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

	uint8_t signal_layer_count[SIGNAL_MAX_DEFINED] = {0};

	for (int32_t id = 0; id < arrlen(PRIVATE(sim)->chips); ++id) {
		Chip *chip = PRIVATE(sim)->chips[id];

		for (uint32_t pin = 0; pin < chip->pin_count; ++pin) {
			// register dependencies
			if (chip->pin_types[pin] & CHIP_PIN_TRIGGER) {
				signal_add_dependency(sim->signal_pool, chip->pins[pin], chip->id);
			}

			// determine which signal layer to use for each pin
			if (chip->pin_types[pin] & CHIP_PIN_OUTPUT) {
				uint8_t signal_layer = signal_layer_count[signal_array_subscript(chip->pins[pin])]++;
				chip->pins[pin].layer  = signal_layer;

				sim->signal_pool->layer_count = MAX(sim->signal_pool->layer_count, (uint32_t) signal_layer + 1);
				sim->signal_pool->block_layer_count[chip->pins[pin].block] =
					MAX(sim->signal_pool->block_layer_count[chip->pins[pin].block], (uint8_t) (signal_layer + 1));
				sim->signal_pool->signals_layer_component[signal_array_subscript(chip->pins[pin])][signal_layer] = id;
			}
		}
	}

	sim->signal_history = signal_history_create(32, sim->signal_pool->signals_count, 256, sim->tick_duration_ps);
}

void simulator_simulate_timestep(Simulator *sim) {
	assert(sim);

	// advance to next timestamp
	if (PRIVATE(sim)->dirty_chips > 0) {
		++sim->current_tick;
	} else {
		// advance to next scheduled event if no chips to be processed
		sim->current_tick = simulator_next_scheduled_event_timestamp(PRIVATE(sim));
	}

	// handle scheduled events for the current timestamp
	sim_handle_event_schedule(PRIVATE(sim));

	// process all chips that have a dependency on signal that was changed in the last timestep or have a scheduled wakeup
	sim_process_sequential(PRIVATE(sim), PRIVATE(sim)->dirty_chips);

	// determine changed signals and dirty chips for next simulation step
	PRIVATE(sim)->dirty_chips = signal_pool_cycle(sim->signal_pool);

	if (sim->signal_history->capture_active) {
		signal_history_add(sim->signal_history, sim->current_tick, sim->signal_pool->signals_value, sim->signal_pool->signals_changed, true);
	}
}

uint64_t simulator_signal_writers(Simulator *sim, Signal signal) {
	assert(sim);

	uint64_t active_chips = 0;
	uint64_t signal_mask = 1ull << signal.index;
	uint64_t signal_subscript = signal_array_subscript(signal);

	for (uint8_t layer = 0; layer < sim->signal_pool->block_layer_count[signal.block]; ++layer) {
		if (sim->signal_pool->signals_next_mask[layer][signal.block] & signal_mask) {
			active_chips |= 1ull << sim->signal_pool->signals_layer_component[signal_subscript][layer];
		}
	}

	return active_chips;
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
		event = (ChipEvent *) dms_malloc(sizeof(ChipEvent));
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

