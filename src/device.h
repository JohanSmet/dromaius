// device.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for devices

#ifndef DROMAIUS_DEVICE_H
#define DROMAIUS_DEVICE_H

#include "chip.h"
#include "signal_line.h"

typedef struct Cpu *(*DEVICE_GET_CPU)(void *device);
typedef void (*DEVICE_PROCESS)(void *device);
typedef void (*DEVICE_RESET)(void *device);
typedef void (*DEVICE_DESTROY)(void *device);
typedef void (*DEVICE_COPY_MEMORY)(void *device, size_t start_address, size_t size, uint8_t *output);

typedef struct ChipEvent {
	int32_t				chip_id;
	int64_t				timestamp;
	struct ChipEvent *	next;
} ChipEvent;

#define DEVICE_DECLARE_FUNCTIONS			\
	DEVICE_GET_CPU get_cpu;					\
	DEVICE_PROCESS process;					\
	DEVICE_RESET reset;						\
	DEVICE_DESTROY destroy;					\
	DEVICE_COPY_MEMORY copy_memory;			\
											\
	SignalPool *			signal_pool;	\
	Chip **					chips;			\
	bool *					chip_is_dirty;	\
	ChipEvent *				event_schedule; \
	ChipEvent *				event_pool;

#define DEVICE_REGISTER_CHIP(name,c)	device_register_chip((Device *) device, (Chip *) (c), (name))

typedef struct Device {
	DEVICE_DECLARE_FUNCTIONS
} Device;

// construction
Chip *device_register_chip(Device *device, Chip *chip, const char *name);
Chip *device_chip_by_name(Device *device, const char *name);

// simulation
void device_simulate_timestep(Device *device);

// scheduler
void device_schedule_event(Device *device, int32_t chip_id, int64_t timestamp);
int32_t device_pop_scheduled_event(Device *device, int64_t timestamp);

static inline int64_t device_next_scheduled_event_timestamp(Device *device) {
	assert(device);
	assert(device->event_schedule);
	return device->event_schedule->timestamp;
}


#endif // DROMAIUS_DEVICE_H
