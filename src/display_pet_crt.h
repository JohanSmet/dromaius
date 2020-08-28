// display_pet_crt.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the PET 2001N 9" CRT Display

#ifndef DROMAIUS_DISPLAY_PET_CRT_H
#define DROMAIUS_DISPLAY_PET_CRT_H

#include "chip.h"
#include "display_rgba.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct DisplayPetCrtSignals {
	Signal	video_in;
	Signal	vert_drive_in;
	Signal	horz_drive_in;
} DisplayPetCrtSignals;

typedef struct DisplayPetCrt {
	CHIP_DECLARE_FUNCTIONS

	SignalPool *			signal_pool;
	DisplayPetCrtSignals	signals;

	DisplayRGBA	*			display;
	uint32_t				pos_x;
	uint32_t				pos_y;

	int64_t					pixel_delta_steps;
	int64_t					vert_overscan_delay;
	int64_t					horz_overscan_delay;
	int64_t					next_action;
} DisplayPetCrt;

// functions
DisplayPetCrt *display_pet_crt_create(SignalPool *pool, DisplayPetCrtSignals signals);
void display_pet_crt_destroy(DisplayPetCrt *crt);
void display_pet_crt_register_dependencies(DisplayPetCrt *crt);
void display_pet_crt_process(DisplayPetCrt *crt);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_DISPLAY_PET_CRT_H
