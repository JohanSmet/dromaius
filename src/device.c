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

void device_simulate_timestep(Device *device) {
	assert(device);

	// advance to next timestamp
	++device->signal_pool->current_tick;

	// process all chips that have a dependency on signal that was changed in the last timestep
	bool work_done = false;

	for (int32_t id = 0; id < arrlen(device->chips); ++id) {
		if (device->chip_is_dirty[id]) {
			work_done = true;
			device->chips[id]->process(device->chips[id]);
		}
	}

	// advance to next schedule event if no chips were processed
	if (!work_done) {
		device->signal_pool->current_tick = device_next_scheduled_event_timestamp(device);
	}

	// handle all events scheduled for the current timestamp
	int32_t chip_id = device_pop_scheduled_event(device, device->signal_pool->current_tick);
	while (chip_id >= 0) {
		device->chips[chip_id]->process(device->chips[chip_id]);
		chip_id = device_pop_scheduled_event(device, device->signal_pool->current_tick);
	}

	// schedule events
	for (int32_t id = 0; id < arrlen(device->chips); ++id) {
		if (device->chips[id]->schedule_timestamp > 0) {
			device_schedule_event(device, id, device->chips[id]->schedule_timestamp);
			device->chips[id]->schedule_timestamp = 0;
		}
	}

	// determine changed signals and dirty chips for next simulation step
	memset(device->chip_is_dirty, false, arrlenu(device->chip_is_dirty));
	signal_pool_cycle_dirty_flags(device->signal_pool, device->chip_is_dirty);
}

void device_schedule_event(Device *device, int32_t chip_id, int64_t timestamp) {
	assert(device);

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

	// insertion sort
	ChipEvent *next = device->event_schedule;
	ChipEvent **prev = &device->event_schedule;

	while (next && next->timestamp < timestamp) {
		prev = &next->next;
		next = next->next;
	}

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
