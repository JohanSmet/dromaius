// device.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for devices

#include "device.h"

#include <stb/stb_ds.h>
#include <assert.h>

Chip *device_register_chip(Device *device, Chip *chip, const char *name) {
	assert(device);
	assert(chip);

	chip->id = (int32_t) arrlenu(device->chips);
	chip->name = name;
	arrpush(device->chips, chip);
	arrpush(device->chip_is_dirty, true);
	arrpush(device->dirty_chips, chip->id);
	return chip;
}

Chip *device_chip_by_name(Device *device, const char *name) {
	assert(device);
	assert(name);

	for (int32_t id = 0; id < arrlen(device->chips); ++id) {
		if (strcmp(device->chips[id]->name, name) == 0) {
			return device->chips[id];
		}
	}

	return NULL;
}

static inline void sim_handle_event_schedule(Device *device) {
	int32_t chip_id = device_pop_scheduled_event(device, device->signal_pool->current_tick);

	while (chip_id >= 0) {
		if (!device->chip_is_dirty[chip_id]) {
			arrpush(device->dirty_chips, chip_id);
			device->chip_is_dirty[chip_id] = true;
		}

		chip_id = device_pop_scheduled_event(device, device->signal_pool->current_tick);
	}
}

static inline void sim_process_sequential(Device *device) {
	for (size_t idx = 0; idx < arrlenu(device->dirty_chips); ++idx) {
		Chip *chip = device->chips[device->dirty_chips[idx]];
		chip->process(chip);

		if (chip->schedule_timestamp > 0) {
			device_schedule_event(device, chip->id, chip->schedule_timestamp);
			chip->schedule_timestamp = 0;
		}
	}
}

void device_simulate_timestep(Device *device) {
	assert(device);

	// advance to next timestamp
	if (arrlenu(device->dirty_chips)) {
		++device->signal_pool->current_tick;
	} else {
		// advance to next scheduled event if no chips to be processed
		device->signal_pool->current_tick = device_next_scheduled_event_timestamp(device);
	}

	// handle scheduled events for the current timestamp
	sim_handle_event_schedule(device);

	// process all chips that have a dependency on signal that was changed in the last timestep or have a scheduled wakeup
	sim_process_sequential(device);

	// determine changed signals and dirty chips for next simulation step
	memset(device->chip_is_dirty, false, arrlenu(device->chip_is_dirty));
	if (device->dirty_chips) {
		stbds_header(device->dirty_chips)->length = 0;
	}

	signal_pool_cycle_dirty_flags(device->signal_pool, device->chip_is_dirty, &device->dirty_chips);
}

void device_schedule_event(Device *device, int32_t chip_id, int64_t timestamp) {
	assert(device);

	// find the insertion spot
	ChipEvent *next = device->event_schedule;
	ChipEvent **prev = &device->event_schedule;

	while (next && next->timestamp < timestamp) {
		if (next->timestamp == timestamp && next->chip_id == chip_id) {
			return;				// no duplicates
		}

		prev = &next->next;
		next = next->next;
	}

	// try to get an allocated event structure from the free pool, or allocate a new one
	ChipEvent *event = device->event_pool;

	if (event) {
		device->event_pool = event->next;
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

int32_t device_pop_scheduled_event(Device *device, int64_t timestamp) {
	assert(device);

	if (!device->event_schedule || device->event_schedule->timestamp != timestamp) {
		return -1;
	}

	ChipEvent *event = device->event_schedule;
	int32_t result = event->chip_id;

	// remove from schedule
	device->event_schedule = event->next;

	// add to reuse pool
	event->next = device->event_pool;
	device->event_pool = event;

	return result;
}
