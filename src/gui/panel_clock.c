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
		.w = 230,
		.h = 120
	};
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR;

	if (nk_begin(nk_ctx, panel_title, panel_bounds, panel_flags)) {
		// state
		nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
		nk_layout_row_push(nk_ctx, 128);
		nk_label(nk_ctx, "Output: ", NK_TEXT_RIGHT);
		nk_layout_row_push(nk_ctx, 32);
		nk_label(nk_ctx, (clock->pin_clock) ? "High" : "Low", NK_TEXT_LEFT);

		// configured frequency
		ui_frequency(nk_ctx, "Target frequency: ", clock->conf_frequency);

		// real frequency
		ui_frequency(nk_ctx, "Real frequency: ", clock->real_frequency);

		// cycle count
		nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
		nk_layout_row_push(nk_ctx, 128);
		nk_label(nk_ctx, "Cycle: ", NK_TEXT_RIGHT);
		nk_layout_row_push(nk_ctx, 74);
		nk_labelf(nk_ctx, NK_TEXT_LEFT, "%ld", clock->cycle_count);
	}
	nk_end(nk_ctx);
}
