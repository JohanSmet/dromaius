// gui/panel_control.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to control the emulator

#include "panel_control.h"
#include "ui_context.h"
#include "dev_minimal_6502.h"

#include <nuklear/nuklear_std.h>

void panel_control(struct nk_context *nk_ctx, UIContext *ui_ctx, struct nk_vec2 pos) {
	static const char *panel_title = "Emulator Control";
	const struct nk_rect panel_bounds = {
		.x = pos.x,
		.y = pos.y,
		.w = 220,
		.h = 70
	};
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR;

	if (nk_begin(nk_ctx, panel_title, panel_bounds, panel_flags)) {
		nk_layout_row_static(nk_ctx, 25, 40, 1);

		if (nk_button_label(nk_ctx, "Step")) {
			dev_minimal_6502_process(ui_ctx->device);
		}
	}
	nk_end(nk_ctx);
}
