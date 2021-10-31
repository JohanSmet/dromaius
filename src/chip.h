// chip.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for chip implementations

#ifndef DROMAIUS_CHIP_H
#define DROMAIUS_CHIP_H

#include "types.h"

typedef enum  {
	CHIP_PIN_01 = 0,
	CHIP_PIN_02 = 1,
	CHIP_PIN_03 = 2,
	CHIP_PIN_04 = 3,
	CHIP_PIN_05 = 4,
	CHIP_PIN_06 = 5,
	CHIP_PIN_07 = 6,
	CHIP_PIN_08 = 7,
	CHIP_PIN_09 = 8,
	CHIP_PIN_10 = 9,
	CHIP_PIN_11 = 10,
	CHIP_PIN_12 = 11,
	CHIP_PIN_13 = 12,
	CHIP_PIN_14 = 13,
	CHIP_PIN_15 = 14,
	CHIP_PIN_16 = 15,
	CHIP_PIN_17 = 16,
	CHIP_PIN_18 = 17,
	CHIP_PIN_19 = 18,
	CHIP_PIN_20 = 19,
	CHIP_PIN_21 = 20,
	CHIP_PIN_22 = 21,
	CHIP_PIN_23 = 22,
	CHIP_PIN_24 = 23,
	CHIP_PIN_25 = 24,
	CHIP_PIN_26 = 25,
	CHIP_PIN_27 = 26,
	CHIP_PIN_28 = 27,
	CHIP_PIN_29 = 28,
	CHIP_PIN_30 = 29,
	CHIP_PIN_31 = 30,
	CHIP_PIN_32 = 31,
	CHIP_PIN_33 = 32,
	CHIP_PIN_34 = 33,
	CHIP_PIN_35 = 34,
	CHIP_PIN_36 = 35,
	CHIP_PIN_37 = 36,
	CHIP_PIN_38 = 37,
	CHIP_PIN_39 = 38,
	CHIP_PIN_40 = 39
} ChipPinNumbers;

typedef void (*CHIP_PROCESS_FUNC)(void *chip);
typedef void (*CHIP_DESTROY_FUNC)(void *chip);
typedef void (*CHIP_DEPENDENCIES_FUNC)(void *chip);

#define CHIP_DECLARE_BASE							\
	CHIP_PROCESS_FUNC process;						\
	CHIP_DESTROY_FUNC destroy;						\
	CHIP_DEPENDENCIES_FUNC register_dependencies;	\
	int32_t			  id;							\
	const char *	  name;							\
	int64_t			  schedule_timestamp;			\
	struct Simulator *simulator;					\
	uint32_t		  signal_layer;					\
	uint32_t		  pin_count;					\
	uint32_t *		  pins;

#define CHIP_SET_FUNCTIONS(chip, pf, df, rdf)		\
	(chip)->process = (CHIP_PROCESS_FUNC) (pf);		\
	(chip)->destroy = (CHIP_DESTROY_FUNC) (df);		\
	(chip)->register_dependencies = (CHIP_DEPENDENCIES_FUNC) (rdf);

#define CHIP_SET_VARIABLES(chip, sim, signals, pc)	\
	(chip)->simulator = (sim);						\
	(chip)->pin_count = (pc);						\
	(chip)->pins = (signals);

typedef struct Chip {
	CHIP_DECLARE_BASE
} Chip;

#endif // DROMAIUS_CHIP_H
