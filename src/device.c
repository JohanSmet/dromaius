// device.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for devices

#include "device.h"

#include <stb/stb_ds.h>
#include <assert.h>

Chip *device_register_chip(Device *device, Chip *chip) {
	assert(device);
	assert(chip);

	chip->id = (int32_t) arrlenu(device->chips);
	arrpush(device->chips, chip);
	return chip;
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
