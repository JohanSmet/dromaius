// gui/panel_control.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to control the emulator

#include "panel_control.h"
#include "ui_context.h"
#include "context.h"
#include "utils.h"
#include "dev_minimal_6502.h"

#include <nuklear/nuklear_std.h>
#include <stb/stb_ds.h>
#include <assert.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

typedef struct PanelControl {
	struct nk_context *		nk_ctx;
	struct DmsContext *		dms_ctx;
	struct nk_rect			bounds;

	const char *			data_path;
	const char **			rom_files;
	const char **			ram_files;

	int						current_rom;
	int						current_ram;
} PanelControl;

///////////////////////////////////////////////////////////////////////////////
//
// private functions
//

static inline char *append_path(const char *path, const char *filename) {
	char *full_path = (char *) malloc(strlen(path) + strlen(filename) + 2);
	strcpy(full_path, path);
	strcat(full_path, "/");
	strcat(full_path, filename);

	return full_path;
}

static void load_current_rom(PanelControl *pnl) {
	assert(pnl);
	
	if (pnl->current_rom < 0) {
		return;
	}

	char *full_path = append_path(pnl->data_path, pnl->rom_files[pnl->current_rom]);
	dev_minimal_6502_rom_from_file(dms_get_device(pnl->dms_ctx), full_path);
	free(full_path);
}

static void load_current_ram(PanelControl *pnl) {
	assert(pnl);
	
	if (pnl->current_ram < 0) {
		return;
	}

	char *full_path = append_path(pnl->data_path, pnl->ram_files[pnl->current_ram]);
	dev_minimal_6502_ram_from_file(dms_get_device(pnl->dms_ctx), full_path);
	free(full_path);
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

PanelControl *panel_control_init(struct nk_context *nk_ctx, struct nk_vec2 pos, struct DmsContext *dms_ctx, const char *data_path) {
	
	PanelControl *pnl = (PanelControl *) malloc(sizeof(PanelControl));
	pnl->nk_ctx = nk_ctx;
	pnl->dms_ctx = dms_ctx;
	pnl->bounds = nk_rect(pos.x, pos.y, 360, 140);

	pnl->data_path = data_path;
	pnl->rom_files = 0;
	pnl->ram_files = 0;
	dir_list_files(pnl->data_path, ".bin", "rom_", &pnl->rom_files);
	dir_list_files(pnl->data_path, ".bin", "ram_", &pnl->ram_files);

	pnl->current_rom = 0;
	pnl->current_ram = 0;
	
	return pnl;
}

void panel_control_release(struct PanelControl *pnl) {
	assert(pnl);

	for (int i = 0; i < arrlen(pnl->rom_files); ++i) {
		free((void *) pnl->rom_files[i]);
	}
	for (int i = 0; i < arrlen(pnl->ram_files); ++i) {
		free((void *) pnl->ram_files[i]);
	}

	arrfree(pnl->rom_files);
	arrfree(pnl->ram_files);
}

void panel_control_display(struct PanelControl *pnl) {
	static const char *panel_title = "Emulator Control";
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR;

	if (nk_begin(pnl->nk_ctx, panel_title, pnl->bounds, panel_flags)) {
		nk_layout_row_static(pnl->nk_ctx, 25, 56, 5);

		if (nk_button_label(pnl->nk_ctx, "Single")) {
			dms_single_step(pnl->dms_ctx);
		}

		if (nk_button_label(pnl->nk_ctx, "Step")) {
			dms_single_instruction(pnl->dms_ctx);
		}

		if (nk_button_label(pnl->nk_ctx, "Run")) {
			dms_run(pnl->dms_ctx);
		}

		if (nk_button_label(pnl->nk_ctx, "Pause")) {
			dms_pause(pnl->dms_ctx);
		}

		if (nk_button_label(pnl->nk_ctx, "Reset")) {
			dms_reset(pnl->dms_ctx);
		}

		nk_layout_row(pnl->nk_ctx, NK_STATIC, 25, 3, (float[]) {80, 200, 56});
		nk_labelf(pnl->nk_ctx, NK_TEXT_RIGHT, "ROM-image: ");
		pnl->current_rom = nk_combo(pnl->nk_ctx, pnl->rom_files, (int) arrlen(pnl->rom_files), pnl->current_rom, 25, nk_vec2(200,200));
		if (nk_button_label(pnl->nk_ctx, "Load")) {
			load_current_rom(pnl);
		}

		nk_layout_row(pnl->nk_ctx, NK_STATIC, 25, 3, (float[]) {80, 200, 56});
		nk_labelf(pnl->nk_ctx, NK_TEXT_RIGHT, "RAM-image: ");
		if (pnl->ram_files != NULL) {
			pnl->current_ram = nk_combo(pnl->nk_ctx, pnl->ram_files, (int) arrlen(pnl->ram_files), pnl->current_ram, 25, nk_vec2(200,200));
			if (nk_button_label(pnl->nk_ctx, "Load")) {
				load_current_ram(pnl);
			}
		} else {
			nk_label(pnl->nk_ctx, "(no images available)", NK_TEXT_LEFT);
		}
	}
	nk_end(pnl->nk_ctx);
}

void panel_control_select_rom(struct PanelControl *pnl, int index) {
	assert(pnl);
	
	if (index < 0) {
		pnl->current_rom = -1;
		return;
	}
	
	if (index < arrlen(pnl->rom_files)) {
		pnl->current_rom = index;
		load_current_rom(pnl);
	}
}

void panel_control_select_ram(struct PanelControl *pnl, int index) {
	assert(pnl);
	
	if (index < 0) {
		pnl->current_ram = -1;
		return; 
	}
	
	if (index < arrlen(pnl->ram_files)) {
		pnl->current_ram = index;
		load_current_ram(pnl);
	}
}
