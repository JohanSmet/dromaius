// perif_pet_crt.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the PET 2001N 9" CRT Display

#ifndef DROMAIUS_PERIF_PET_CRT_H
#define DROMAIUS_PERIF_PET_CRT_H

#include "chip.h"
#include "display_rgba.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
enum PerifPetCrtSignalAssignment {
	PIN_PETCRT_VIDEO_IN		 = CHIP_PIN_01,
	PIN_PETCRT_VERT_DRIVE_IN = CHIP_PIN_02,
	PIN_PETCRT_HORZ_DRIVE_IN = CHIP_PIN_03
};

#define CHIP_PETCRT_PIN_COUNT 3
typedef Signal PerifPetCrtSignals[CHIP_PETCRT_PIN_COUNT];

typedef struct PerifPetCrt {
	CHIP_DECLARE_BASE

	SignalPool *			signal_pool;
	PerifPetCrtSignals		signals;

	DisplayRGBA	*			display;
	uint32_t				pos_x;
	uint32_t				pos_y;

	int64_t					pixel_delta_steps;
	int64_t					vert_overscan_delay;
	int64_t					horz_overscan_delay;
	int64_t					next_action;
} PerifPetCrt;

// functions
PerifPetCrt *perif_pet_crt_create(struct Simulator *sim, PerifPetCrtSignals signals);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_PERIF_PET_CRT_H
