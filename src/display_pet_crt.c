// display_pet_crt.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the PET 2001N 9" CRT Display

#include "display_pet_crt.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SIGNAL_POOL			crt->signal_pool
#define SIGNAL_COLLECTION	crt->signals
#define SIGNAL_CHIP_ID		crt->id

#define COLOR_ON	0xff55ff55
#define COLOR_BEAM  0xffffffff

DisplayPetCrt *display_pet_crt_create(SignalPool *pool, DisplayPetCrtSignals signals) {
	DisplayPetCrt *crt = (DisplayPetCrt *) calloc(1, sizeof(DisplayPetCrt));

	// chip
	CHIP_SET_FUNCTIONS(crt, display_pet_crt_process, display_pet_crt_destroy, display_pet_crt_register_dependencies);

	// signals
	crt->signal_pool = pool;
	memcpy(&crt->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(video_in, 1);
	SIGNAL_DEFINE(vert_drive_in, 1);
	SIGNAL_DEFINE(horz_drive_in, 1);

	// display
	size_t width = 40 * 8;
	size_t height = 25 * 8 + 4;

	crt->display = display_rgba_create(width, height);

	crt->pixel_delta_steps = signal_pool_interval_to_tick_count(pool, NS_TO_PS(125));			// 8 Mhz

	// delays to skip the blank parts at the top and left (or right) of the screen
	crt->vert_overscan_delay = signal_pool_interval_to_tick_count(crt->signal_pool, US_TO_PS(1225));
	crt->horz_overscan_delay = signal_pool_interval_to_tick_count(crt->signal_pool, NS_TO_PS(18500 + 30));

	return crt;
}

void display_pet_crt_destroy(DisplayPetCrt *crt) {
	assert(crt);
	display_rgba_destroy(crt->display);
	free(crt);
}

void display_pet_crt_register_dependencies(DisplayPetCrt *crt) {
	assert(crt);
	signal_add_dependency(crt->signal_pool, SIGNAL(horz_drive_in), crt->id);
	signal_add_dependency(crt->signal_pool, SIGNAL(vert_drive_in), crt->id);
}

void display_pet_crt_process(DisplayPetCrt *crt) {
	assert(crt);


	// in vertical retrace?
	if (!SIGNAL_BOOL(vert_drive_in)) {
		crt->pos_y = 0;
		return;
	}

	if (signal_changed_last_tick(crt->signal_pool, SIGNAL(vert_drive_in))) {
		crt->schedule_timestamp = crt->signal_pool->current_tick + crt->vert_overscan_delay;
		crt->next_action = MAX(crt->next_action, crt->schedule_timestamp);
	}

	// positive transition on horzontal drive signal? (== start horizontal retrace)
	if (SIGNAL_BOOL(horz_drive_in) && signal_changed_last_tick(crt->signal_pool, SIGNAL(horz_drive_in))) {
		if (crt->pos_x > 0) {
			crt->pos_x = 0;
			crt->pos_y += 1;

			// to account for the time it takes the beam to move back to the left
			crt->schedule_timestamp = crt->signal_pool->current_tick + crt->horz_overscan_delay;
			crt->next_action = MAX(crt->next_action, crt->schedule_timestamp);
			return;
		}
	}

	if (crt->next_action > crt->signal_pool->current_tick) {
		return;
	}

	if (crt->pos_y < crt->display->height && crt->pos_x < crt->display->width) {
		crt->display->frame[(crt->pos_y * crt->display->width) + crt->pos_x] = (SIGNAL_BOOL(video_in) ? 0 : COLOR_ON);
	}
	crt->pos_x += 1;
	crt->schedule_timestamp = crt->signal_pool->current_tick + crt->pixel_delta_steps;
	crt->next_action = crt->schedule_timestamp;
}
