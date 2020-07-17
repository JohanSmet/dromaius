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

