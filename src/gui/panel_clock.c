// gui/panel_clock.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_clock.h"
#include "ui_context.h"

#include "clock.h"

#include <nuklear/nuklear_std.h>

static inline void ui_frequency(struct nk_context *nk_ctx, const char *label, int64_t freq) {

	nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
	nk_layout_row_push(nk_ctx, 128);
	nk_label(nk_ctx, label, NK_TEXT_RIGHT);
	nk_layout_row_push(nk_ctx, 74);
	if (freq > 1000000) {
		nk_labelf(nk_ctx, NK_TEXT_LEFT, "%.3f MHz", freq / 1000000.0f);
	} else if (freq > 1000) {
		nk_labelf(nk_ctx, NK_TEXT_LEFT, "%.3f KHz", freq / 1000.0f);
	} else {
		nk_labelf(nk_ctx, NK_TEXT_LEFT, "%ld Hz", freq);
	}
}

void panel_clock(struct nk_context *nk_ctx, Clock *clock, struct nk_vec2 pos) {
	static const char *panel_title = "Clock";
	const struct nk_rect panel_bounds = {
		.x = pos.x,
		.y = pos.y,
		.w = 300,
		.h = 125
	};
	static const char *units[] = {"Hz", "KHz", "MHz"};
	static const int factor[] = {1, 1000, 1000000};
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR;

	if (nk_begin(nk_ctx, panel_title, panel_bounds, panel_flags)) {

		// configured frequency
		int unit_idx = 0;
		float value = clock->conf_frequency;

		if (value >= 1000000) {
			value /= 1000000;
			unit_idx = 2;
		} else if (value >= 1000) {
			value /= 1000;
			unit_idx = 1;
		}

		nk_layout_row(nk_ctx, NK_STATIC, 25, 2, (float[]) {200, 74});
		nk_property_float(nk_ctx, "Target frequency", 1, &value, 999, 1, 1);
		nk_combobox(nk_ctx, units, NK_LEN(units), &unit_idx, 25, nk_vec2(60,200));

		if (clock->conf_frequency != value * factor[unit_idx]) {
			clock_set_frequency(clock, value * factor[unit_idx]);
		}

		// real frequency
		ui_frequency(nk_ctx, "Real frequency: ", clock->real_frequency);

		// state
		nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
		nk_layout_row_push(nk_ctx, 128);
		nk_label(nk_ctx, "Output: ", NK_TEXT_RIGHT);
		nk_layout_row_push(nk_ctx, 32);
		nk_label(nk_ctx, (clock->pin_clock) ? "High" : "Low", NK_TEXT_LEFT);

		// cycle count
		nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
		nk_layout_row_push(nk_ctx, 128);
		nk_label(nk_ctx, "Cycle: ", NK_TEXT_RIGHT);
		nk_layout_row_push(nk_ctx, 74);
		nk_labelf(nk_ctx, NK_TEXT_LEFT, "%ld", clock->cycle_count);
	}
	nk_end(nk_ctx);
}
