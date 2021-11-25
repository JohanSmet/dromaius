// chip.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for chip implementations

#ifndef DROMAIUS_CHIP_H
#define DROMAIUS_CHIP_H

#include "types.h"
#include "signal_types.h"

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

typedef enum {
	CHIP_PIN_INPUT   = 0b00000001,				// chips reads from this pin
	CHIP_PIN_OUTPUT  = 0b00000010,				// chips writes to this pin
	CHIP_PIN_TRIGGER = 0b00000100,				// processing is triggered by changes on this pin
} ChipPinType;

typedef void (*CHIP_PROCESS_FUNC)(void *chip);
typedef void (*CHIP_DESTROY_FUNC)(void *chip);

#define CHIP_DECLARE_BASE							\
	CHIP_PROCESS_FUNC process;						\
	CHIP_DESTROY_FUNC destroy;						\
	int32_t			  id;							\
	const char *	  name;							\
	int64_t			  schedule_timestamp;			\
	struct Simulator *simulator;					\
	uint32_t		  pin_count;					\
	Signal *		  pins;							\
	uint8_t *		  pin_types;

#define CHIP_SET_FUNCTIONS(chip, pf, df)			\
	(chip)->process = (CHIP_PROCESS_FUNC) (pf);		\
	(chip)->destroy = (CHIP_DESTROY_FUNC) (df);

#define CHIP_SET_VARIABLES(chip, sim, signals, pt, pc)		\
	(chip)->simulator = (sim);								\
	(chip)->pin_count = (pc);								\
	(chip)->pins = (signals);								\
	(chip)->pin_types = (pt);

typedef struct Chip {
	CHIP_DECLARE_BASE
} Chip;

#endif // DROMAIUS_CHIP_H
