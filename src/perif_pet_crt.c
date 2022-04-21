// perif_pet_crt.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the PET 2001N 9" CRT Display

#include "perif_pet_crt.h"
#include "simulator.h"
#include "crt.h"

#define SIGNAL_PREFIX		PIN_PETCRT_
#define SIGNAL_OWNER		crt

static uint8_t PerifPetCrt_PinTypes[CHIP_PETCRT_PIN_COUNT] = {
	[PIN_PETCRT_VIDEO_IN     ] = CHIP_PIN_INPUT,
	[PIN_PETCRT_VERT_DRIVE_IN] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[PIN_PETCRT_HORZ_DRIVE_IN] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
};

#define COLOR_ON	0xff55ff55
#define COLOR_OFF	0xff111111

static void perif_pet_crt_destroy(PerifPetCrt *crt);
static void perif_pet_crt_process(PerifPetCrt *crt);

PerifPetCrt *perif_pet_crt_create(Simulator *sim, PerifPetCrtSignals signals) {
	PerifPetCrt *crt = (PerifPetCrt *) dms_calloc(1, sizeof(PerifPetCrt));

	// chip
	CHIP_SET_FUNCTIONS(crt, perif_pet_crt_process, perif_pet_crt_destroy);
	CHIP_SET_VARIABLES(crt, sim, crt->signals, PerifPetCrt_PinTypes, CHIP_PETCRT_PIN_COUNT);

	// signals
	crt->signal_pool = sim->signal_pool;
	dms_memcpy(crt->signals, signals, sizeof(PerifPetCrtSignals));
	SIGNAL_DEFINE(VIDEO_IN);
	SIGNAL_DEFINE(VERT_DRIVE_IN);
	SIGNAL_DEFINE(HORZ_DRIVE_IN);

	// display
	size_t width = 40 * 8;
	size_t height = 25 * 8 + 4;

	crt->display = display_rgba_create(width, height);

	crt->pixel_delta_steps = simulator_interval_to_tick_count(crt->simulator, NS_TO_PS(125));			// 8 Mhz

	// delays to skip the blank parts at the top and left (or right) of the screen
	crt->vert_overscan_delay = simulator_interval_to_tick_count(crt->simulator, US_TO_PS(1225));
	crt->horz_overscan_delay = simulator_interval_to_tick_count(crt->simulator, NS_TO_PS(18500 + 30));

	return crt;
}

static void perif_pet_crt_destroy(PerifPetCrt *crt) {
	assert(crt);
	display_rgba_destroy(crt->display);
	dms_free(crt);
}

static void perif_pet_crt_process(PerifPetCrt *crt) {
	assert(crt);

	static const uint32_t COLORS[2] = { COLOR_ON, COLOR_OFF };

	// in vertical retrace?
	if (!SIGNAL_READ(VERT_DRIVE_IN)) {
		crt->pos_y = 0;
		return;
	}

	if (SIGNAL_CHANGED(VERT_DRIVE_IN)) {
		crt->schedule_timestamp = crt->simulator->current_tick + crt->vert_overscan_delay;
		crt->next_action = MAX(crt->next_action, crt->schedule_timestamp);
	}

	// positive transition on horzontal drive signal? (== start horizontal retrace)
	if (SIGNAL_READ(HORZ_DRIVE_IN) && SIGNAL_CHANGED(HORZ_DRIVE_IN)) {
		if (crt->pos_x > 0) {
			crt->pos_x = 0;
			crt->pos_y += 1;

			// to account for the time it takes the beam to move back to the left
			crt->schedule_timestamp = crt->simulator->current_tick + crt->horz_overscan_delay;
			crt->next_action = MAX(crt->next_action, crt->schedule_timestamp);
			return;
		}
	}

	if (crt->next_action > crt->simulator->current_tick) {
		return;
	}

	if (crt->pos_y < crt->display->height && crt->pos_x < crt->display->width) {
		crt->display->frame[(crt->pos_y * crt->display->width) + crt->pos_x] = COLORS[SIGNAL_READ(VIDEO_IN)];
	}
	crt->pos_x += 1;
	crt->schedule_timestamp = crt->simulator->current_tick + crt->pixel_delta_steps;
	crt->next_action = crt->schedule_timestamp;
}
