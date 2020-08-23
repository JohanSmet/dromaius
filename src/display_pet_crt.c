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
	size_t height = 25 * 8;

	crt->display = display_rgba_create(width, height);

	// the crt takes 24us to scan on horizontal line
	crt->pixel_delta_steps = (int) signal_pool_interval_to_tick_count(pool, US_TO_PS(24) / (int) width);
	crt->schedule_timestamp = crt->pixel_delta_steps;

	crt->display->frame[0] = COLOR_BEAM;

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

	bool horz_drive = SIGNAL_BOOL(horz_drive_in);
	bool vert_drive = SIGNAL_BOOL(vert_drive_in);

	//printf("%lld: last = %lld, horz = %d, vert = %d, x = %d, y = %d => ",
			//crt->signal_pool->current_tick, crt->last_pixel_transition, horz_drive, vert_drive, crt->pos_x, crt->pos_y);

	if (horz_drive && vert_drive) {
		if (signal_changed_last_tick(crt->signal_pool, SIGNAL(horz_drive_in)) ||
			signal_changed_last_tick(crt->signal_pool, SIGNAL(vert_drive_in))) {
			//printf(" reset ");
			crt->last_pixel_transition = crt->signal_pool->current_tick;
		}
		while (crt->last_pixel_transition + crt->pixel_delta_steps <= crt->signal_pool->current_tick) {
			crt->pos_x += 1;
			crt->last_pixel_transition += crt->pixel_delta_steps;

			if (crt->pos_y < crt->display->height && crt->pos_x < crt->display->width) {
				crt->display->frame[(crt->pos_y * crt->display->width) + crt->pos_x] = (SIGNAL_BOOL(video_in) ? 0 : COLOR_ON);
			}
		}

		crt->schedule_timestamp = crt->last_pixel_transition + crt->pixel_delta_steps;
	}

	if (!horz_drive) {
		if (crt->pos_x > 0) {
			crt->pos_y += 1;
			crt->pos_x = 0;
		}
		crt->last_pixel_transition = crt->signal_pool->current_tick;
	}

	if (!vert_drive) {
		crt->pos_x = 0;
		crt->pos_y = 0;
		crt->last_pixel_transition = crt->signal_pool->current_tick;
	}

	//printf("x = %d, y = %d\n", crt->pos_x, crt->pos_y);
	//crt->display->frame[(crt->pos_y * crt->display->width) + crt->pos_x] = COLOR_ON;


}
