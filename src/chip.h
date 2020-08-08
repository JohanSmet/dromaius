// chip.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for chip implementations

#ifndef DROMAIUS_CHIP_H
#define DROMAIUS_CHIP_H

#include "types.h"

typedef void (*CHIP_PROCESS_FUNC)(void *chip);
typedef void (*CHIP_DESTROY_FUNC)(void *chip);
typedef void (*CHIP_DEPENDENCIES_FUNC)(void *chip);

#define CHIP_DECLARE_FUNCTIONS		\
	CHIP_PROCESS_FUNC process;		\
	CHIP_DESTROY_FUNC destroy;		\
	CHIP_DEPENDENCIES_FUNC register_dependencies;	\
	int32_t			  id;							\
	const char *	  name;							\
	int64_t			  schedule_timestamp;

#define CHIP_SET_FUNCTIONS(chip, pf, df, rdf)		\
	(chip)->process = (CHIP_PROCESS_FUNC) (pf);		\
	(chip)->destroy = (CHIP_DESTROY_FUNC) (df);		\
	(chip)->register_dependencies = (CHIP_DEPENDENCIES_FUNC) (rdf);

typedef struct Chip {
	CHIP_DECLARE_FUNCTIONS
} Chip;

#endif // DROMAIUS_CHIP_H
