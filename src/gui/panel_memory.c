// gui/panel_memory.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI-panel to visualize a block of memory

#include "panel_memory.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <nuklear/nuklear_std.h>
#include <stb/stb_ds.h>

#include "filt_6502_asm.h"

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

enum DISPLAY_TYPES {
	DT_RAW = 0, 
	DT_DISASM = 1
};

typedef struct PanelMemory {
	struct nk_context * nk_ctx;
	struct nk_rect		bounds;
	char *				title;

	const uint8_t *		mem;
	size_t				mem_size;
	size_t				mem_offset;

	int					display_type;
	int					follow_pc;
	uint64_t			last_pc;
} PanelMemory;

///////////////////////////////////////////////////////////////////////////////
//
// private functions
//

static inline void ui_spacer(struct nk_context *nk_ctx, int width) {
	nk_layout_row_push(nk_ctx, (float) width);
	nk_label(nk_ctx, "", NK_TEXT_LEFT);
}

static inline void ui_static(struct nk_context *nk_ctx, int width, const char *fmt, ...) {
    va_list args;
	nk_layout_row_push(nk_ctx, (float) width);
    va_start(args, fmt);
    nk_labelfv(nk_ctx, NK_TEXT_LEFT, fmt, args);
    va_end(args);
}

static inline void memory_display_raw(PanelMemory *pnl, int width, int height) {
	int index = 0;

	nk_layout_row_static(pnl->nk_ctx, (float) height, width - 10, 1);

	if (nk_group_begin(pnl->nk_ctx, "client", 0)) {

		while (index < pnl->mem_size) {
			nk_layout_row_begin(pnl->nk_ctx, NK_STATIC, 18, 20);

			ui_static(pnl->nk_ctx, 40, "%.4x:", pnl->mem_offset + index);

			for (int i = 0; i < 4 && index < pnl->mem_size; ++i) {
				ui_static(pnl->nk_ctx, 16, "%.2x", pnl->mem[index++]);
			}

			ui_spacer(pnl->nk_ctx, 2);

			for (int i = 0; i < 4 && index < pnl->mem_size; ++i) {
				ui_static(pnl->nk_ctx, 16, "%.2x", pnl->mem[index++]);
			}

			ui_spacer(pnl->nk_ctx, 4);

			for (int i = 0; i < 4 && index < pnl->mem_size; ++i) {
				ui_static(pnl->nk_ctx, 16, "%.2x", pnl->mem[index++]);
			}

			ui_spacer(pnl->nk_ctx, 2);

			for (int i = 0; i < 4 && index < pnl->mem_size; ++i) {
				ui_static(pnl->nk_ctx, 16, "%.2x", pnl->mem[index++]);
			}
		}

		nk_group_end(pnl->nk_ctx);
	}
}

static inline void memory_display_disasm_6502(PanelMemory *pnl, int width, int height, uint64_t current_pc) {

	static const char *symbols[] = {"", ">"};
	const struct nk_color colors[] = { pnl->nk_ctx->style.text.color, nk_rgb(255,255,0)};

	const int lbl_h = 18;
	const int row_h = lbl_h + (int) pnl->nk_ctx->style.window.spacing.y;

	if (pnl->follow_pc && pnl->last_pc != current_pc &&
		current_pc > pnl->mem_offset && current_pc < pnl->mem_offset + pnl->mem_size) {
		pnl->last_pc = current_pc;
		int lines = filt_6502_asm_count_instruction(pnl->mem, pnl->mem_size, 0, current_pc - pnl->mem_offset);
		int offset = row_h * lines - ((height - 30) >> 1);
		nk_uint y_ofs = (offset > 0) ? offset : 0;
		nk_group_set_scroll(pnl->nk_ctx, "client", 0, y_ofs);
	}

	nk_layout_row_static(pnl->nk_ctx, (float) height - 24, width - 10, 1);

	if (nk_group_begin(pnl->nk_ctx, "client", 0)) {

		int index = 0;
		char *line = NULL;

		while (index < pnl->mem_size) {
			bool is_current = (index + pnl->mem_offset) == current_pc;

			index += filt_6502_asm_line(pnl->mem, pnl->mem_size, index, pnl->mem_offset, &line);

			nk_layout_row(pnl->nk_ctx, NK_STATIC, (float) lbl_h, 2, (float[]) {6, 200});
			nk_label_colored(pnl->nk_ctx, symbols[is_current], NK_TEXT_RIGHT, colors[is_current]);
			nk_label_colored(pnl->nk_ctx, line, NK_TEXT_LEFT, colors[is_current]);
			arrsetlen(line, 0);
		}
		nk_group_end(pnl->nk_ctx);
	}

	nk_layout_row(pnl->nk_ctx, NK_STATIC, 20, 1, (float[]) {60});
	nk_checkbox_label(pnl->nk_ctx, "Follow PC", &pnl->follow_pc);
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

struct PanelMemory *panel_memory_init( 
						struct nk_context *nk_ctx, 
						struct nk_vec2 pos, const char *title, 
						const uint8_t *data, size_t data_size, size_t data_offset) {
	
	PanelMemory *pnl = (PanelMemory *) malloc(sizeof(PanelMemory));
	pnl->nk_ctx = nk_ctx;
	pnl->bounds = nk_rect(pos.x, pos.y, 440, 220);
	pnl->title = strdup(title);

	pnl->mem = data;
	pnl->mem_size = data_size;
	pnl->mem_offset = data_offset;

	pnl->display_type = DT_RAW;
	pnl->follow_pc = true;
	pnl->last_pc = 0;

	return pnl;
}

void panel_memory_display(struct PanelMemory *pnl, uint64_t current_pc) {
	assert(pnl);

	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE;
	static const char *display_types[] = {"Raw", "6502 Disassembly"};

	if (nk_begin(pnl->nk_ctx, pnl->title, pnl->bounds, panel_flags)) {
		nk_layout_row(pnl->nk_ctx, NK_STATIC, 24, 2, (float[]) {100, 200});
		nk_label(pnl->nk_ctx, "Display type: ", NK_TEXT_RIGHT);
		pnl->display_type = nk_combo(pnl->nk_ctx, display_types, NK_LEN(display_types), pnl->display_type, 24, nk_vec2(200,200));
		
		int w = (int) nk_window_get_width(pnl->nk_ctx) - 16;
		int h = (int) nk_window_get_height(pnl->nk_ctx) - 24 - 56;

		switch (pnl->display_type) {
			case DT_RAW:
				memory_display_raw(pnl, w, h);
				break;
			case DT_DISASM:
				memory_display_disasm_6502(pnl, w, h, current_pc);
				break;
		} 

	}
	nk_end(pnl->nk_ctx);
}

void panel_memory_release(struct PanelMemory *pnl) {
	assert(pnl);
	free(pnl->title);
}


