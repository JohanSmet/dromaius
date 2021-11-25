// simulator.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Core simulation functions

#include "simulator.h"
#include "chip.h"
#include "signal_line.h"

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

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Simulator *simulator_create(int64_t tick_duration_ps) {
	Simulator_private *priv_sim = (Simulator_private *) calloc(1, sizeof(Simulator_private));

	PUBLIC(priv_sim)->signal_pool = signal_pool_create();
	PUBLIC(priv_sim)->tick_duration_ps = tick_duration_ps;

	return &priv_sim->public;
}

void simulator_destroy(Simulator *sim) {
	assert(sim);

	for (int32_t id = 0; id < arrlen(PRIVATE(sim)->chips); ++id) {
		PRIVATE(sim)->chips[id]->destroy(PRIVATE(sim)->chips[id]);
	}

	arrfree(PRIVATE(sim)->chips);

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
		Chip *chip = PRIVATE(sim)->chips[id];

		for (uint32_t pin = 0; pin < chip->pin_count; ++pin) {
			if (chip->pin_types[pin] & CHIP_PIN_TRIGGER) {
				signal_add_dependency(sim->signal_pool, chip->pins[pin], chip->id);
			}
		}
	}

	// determine what signal-layer each chip should write to
	memset(sim->signal_pool->block_layers, 0, sizeof(uint64_t) * SIGNAL_BLOCKS);

	for (int32_t chip_id = 0; chip_id < arrlen(PRIVATE(sim)->chips); ++chip_id) {
		Chip *chip = PRIVATE(sim)->chips[chip_id];

		// check which layers are already in use on signals connected to this chip
		uint64_t used_layers = 0;

		for (uint32_t pin_idx = 0; pin_idx < chip->pin_count; ++pin_idx) {
			if (!signal_is_undefined(chip->pins[pin_idx])) {
				used_layers |= sim->signal_pool->signals_layers[signal_array_subscript(chip->pins[pin_idx])];
			}
		}

		// use lowest available layer
		uint32_t signal_layer = (uint32_t) bit_lowest_set(~used_layers);
		sim->signal_pool->layer_count = MAX(sim->signal_pool->layer_count, signal_layer + 1);

		// mark layer as used on all pins
		uint64_t layer_mask = 1ull << signal_layer;

		for (uint32_t pin_idx = 1; pin_idx < chip->pin_count; ++pin_idx) {
			chip->pins[pin_idx].layer = (uint8_t) signal_layer;
			sim->signal_pool->signals_layers[signal_array_subscript(chip->pins[pin_idx])] |= layer_mask;
			sim->signal_pool->block_layers[chip->pins[pin_idx].block] |= layer_mask;
		}


		// mark chip as used on the layer
		PRIVATE(sim)->layer_chips[signal_layer] |= 1ull << chip_id;
	}
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
}

uint64_t simulator_signal_writers(Simulator *sim, Signal signal) {
	assert(sim);

	uint64_t signal_flag = 1ull << signal.index;
	uint64_t result = 0;

	// check all layers that are used by this signal
	for (uint64_t layers = sim->signal_pool->signals_layers[signal_array_subscript(signal)]; layers; layers &= layers - 1) {

		int32_t layer = bit_lowest_set(layers);

		// skip if no write on this layer
		if (!FLAG_IS_SET(sim->signal_pool->signals_next_mask[layer][signal.block], signal_flag)) {
			continue;
		}

		// check all chips that use this layer
		for (uint64_t layer_chips = PRIVATE(sim)->layer_chips[layer]; layer_chips; layer_chips &= layer_chips - 1) {
			int32_t chip_id = bit_lowest_set(layer_chips);
			Chip *chip = PRIVATE(sim)->chips[chip_id];

			for (uint32_t pin_idx = 0; pin_idx < chip->pin_count; ++pin_idx) {
				if (signal_equal(chip->pins[pin_idx], signal)) {
					result |= 1ull << chip_id;
				}
			}
		}
	}

	return result;
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

