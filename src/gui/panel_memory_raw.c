// gui/panel_memory_raw.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display raw bytes of memory

#include "panel_memory_raw.h"
#include <nuklear/nuklear_std.h>

static inline void ui_spacer(struct nk_context *nk_ctx, int width) {
	nk_layout_row_push(nk_ctx, width);
	nk_label(nk_ctx, "", NK_TEXT_LEFT);
}

static inline void ui_static(struct nk_context *nk_ctx, int width, const char *fmt, ...) {
    va_list args;
	nk_layout_row_push(nk_ctx, width);
    va_start(args, fmt);
    nk_labelfv(nk_ctx, NK_TEXT_LEFT, fmt, args);
    va_end(args);
}

void panel_memory_raw(struct nk_context *nk_ctx, struct nk_vec2 pos, const char *title, 
		              uint8_t *data, size_t data_size, size_t data_offset) {

	const struct nk_rect panel_bounds = {
		.x = pos.x,
		.y = pos.y,
		.w = 410,
		.h = 220
	};
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE;

	if (nk_begin(nk_ctx, title, panel_bounds, panel_flags)) {
		
		int index = 0;

		while (index < data_size) {
			nk_layout_row_begin(nk_ctx, NK_STATIC, 18, 20);

			ui_static(nk_ctx, 40, "%.4x:", data_offset + index);

			for (int i = 0; i < 4 && index < data_size; ++i) {
				ui_static(nk_ctx, 16, "%.2x", data[index++]);
			}

			ui_spacer(nk_ctx, 2);

			for (int i = 0; i < 4 && index < data_size; ++i) {
				ui_static(nk_ctx, 16, "%.2x", data[index++]);
			}

			ui_spacer(nk_ctx, 4);

			for (int i = 0; i < 4 && index < data_size; ++i) {
				ui_static(nk_ctx, 16, "%.2x", data[index++]);
			}

			ui_spacer(nk_ctx, 2);

			for (int i = 0; i < 4 && index < data_size; ++i) {
				ui_static(nk_ctx, 16, "%.2x", data[index++]);
			}
		}
	}
	nk_end(nk_ctx);
}

