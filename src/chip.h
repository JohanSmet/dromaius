// chip.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for chip implementations

#ifndef DROMAIUS_CHIP_H
#define DROMAIUS_CHIP_H

#include "types.h"

typedef void (*CHIP_PROCESS_FUNC)(void *chip);
typedef void (*CHIP_DESTROY_FUNC)(void *chip);

#define CHIP_DECLARE_FUNCTIONS		\
	CHIP_PROCESS_FUNC process;		\
	CHIP_DESTROY_FUNC destroy;

#define CHIP_SET_FUNCTIONS(chip, pf, df)		\
	(chip)->process = (CHIP_PROCESS_FUNC) (pf);	\
	(chip)->destroy = (CHIP_DESTROY_FUNC) (df);

typedef struct Chip {
	CHIP_DECLARE_FUNCTIONS
} Chip;

#endif // DROMAIUS_CHIP_H
