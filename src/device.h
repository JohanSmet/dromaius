// device.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for devices

#ifndef DROMAIUS_DEVICE_H
#define DROMAIUS_DEVICE_H

#include "chip.h"
#include "signal_line.h"

typedef struct Cpu *(*DEVICE_GET_CPU)(void *device);
typedef struct Clock *(*DEVICE_GET_CLOCK)(void *device);
typedef void (*DEVICE_PROCESS)(void *device);
typedef void (*DEVICE_RESET)(void *device);
typedef void (*DEVICE_DESTROY)(void *device);

#define DEVICE_DECLARE_FUNCTIONS			\
	DEVICE_GET_CPU get_cpu;					\
	DEVICE_GET_CLOCK get_clock;				\
	DEVICE_PROCESS process;					\
	DEVICE_RESET reset;						\
	DEVICE_DESTROY destroy;					\
											\
	SignalPool *			signal_pool;	\
	Chip **					chips;

typedef struct Device {
	DEVICE_DECLARE_FUNCTIONS
} Device;

Chip *device_register_chip(Device *device, Chip *chip);

#endif // DROMAIUS_DEVICE_H
