// gui/panel_memory_disasm.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display a disassembly of a memory block

#include "panel_memory_disasm.h"

#include <nuklear/nuklear_std.h>
#include <stb/stb_ds.h>

#include "filt_6502_asm.h"
#include "nuklear/nuklear.h"

void panel_memory_disasm(struct nk_context *nk_ctx, struct UIContext *ui_ctx, struct nk_vec2 pos, const char *title, 
						 uint8_t *data, size_t data_size, size_t data_offset, uint64_t pc) {

	const struct nk_rect panel_bounds = {
		.x = pos.x,
		.y = pos.y,
		.w = 240,
		.h = 220
	};
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE;

	static const char *symbols[] = {"", ">"};
	const struct nk_color colors[] = { nk_ctx->style.text.color, nk_rgb(255,255,0)};

	if (nk_begin(nk_ctx, title, panel_bounds, panel_flags)) {
		
		int index = 0;
		char *line = NULL;

		while (index < data_size) {
			bool is_current = (index + data_offset) == pc;

			index += filt_6502_asm_line(data, data_size, index, data_offset, &line);

			nk_layout_row(nk_ctx, NK_STATIC, 18, 2, (float[]) {6, 200});
			nk_label_colored(nk_ctx, symbols[is_current], NK_TEXT_RIGHT, colors[is_current]);
			nk_label_colored(nk_ctx, line, NK_TEXT_LEFT, colors[is_current]);
			arrsetlen(line, 0);
		}
	}
	nk_end(nk_ctx);

}
