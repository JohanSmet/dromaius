// device.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for devices

#ifndef DROMAIUS_DEVICE_H
#define DROMAIUS_DEVICE_H

#include "chip.h"
#include "simulator.h"
#include "signal_types.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Cpu *(*DEVICE_GET_CPU)(void *device);
typedef void (*DEVICE_PROCESS)(void *device);
typedef void (*DEVICE_RESET)(void *device);
typedef void (*DEVICE_DESTROY)(void *device);
typedef void (*DEVICE_READ_MEMORY)(void *device, size_t start_address, size_t size, uint8_t *output);
typedef void (*DEVICE_WRITE_MEMORY)(void *device, size_t start_address, size_t size, uint8_t *input);
typedef size_t (*DEVICE_GET_IRQ_SIGNALS)(void *device, SignalBreakpoint **irq_signals);

#define DEVICE_DECLARE_FUNCTIONS			\
	DEVICE_GET_CPU get_cpu;					\
	DEVICE_PROCESS process;					\
	DEVICE_RESET reset;						\
	DEVICE_DESTROY destroy;					\
	DEVICE_READ_MEMORY read_memory;			\
	DEVICE_WRITE_MEMORY write_memory;		\
	DEVICE_GET_IRQ_SIGNALS get_irq_signals; \
											\
	struct Simulator *simulator;

#define DEVICE_REGISTER_CHIP(name,c)	simulator_register_chip(device->simulator, (Chip *) (c), (name))

typedef struct Device {
	DEVICE_DECLARE_FUNCTIONS
} Device;

// simulation
void device_process(Device *device);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_DEVICE_H
