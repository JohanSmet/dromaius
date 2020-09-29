// device.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for devices

#include "device.h"
#include <assert.h>

void device_process(Device *device) {
	assert(device);
	simulator_simulate_timestep(device->simulator);
}
