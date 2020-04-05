// chip.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for chip implementations

#ifndef DROMAIUS_CHIP_H
#define DROMAIUS_CHIP_H

#include "types.h"

typedef void (*PROCESS_FUNC)(void *chip);

#define CHIP_DECLARE_FUNCTIONS		\
	PROCESS_FUNC process;

typedef struct Chip {
	CHIP_DECLARE_FUNCTIONS
} Chip;

#endif // DROMAIUS_CHIP_H
