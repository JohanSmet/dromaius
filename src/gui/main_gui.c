// gui/main_gui.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include <nuklear/nuklear.h>

#include "dev_minimal_6502.h"
#include "filt_6502_asm.h"

#include "utils.h"
#include <stb/stb_ds.h>

typedef struct UIContext {
	DevMinimal6502 *device;
	uint8_t *rom_data;

} UIContext;

UIContext ui_context;

void panel_memory_assembly(struct nk_context *nk_ctx, UIContext *ui_ctx);

char *nuklear_config_window_title(void) {
	return "Dromaius";
}

void nuklear_on_start(struct nk_context *ctx) {
	// load ROM
	ui_context.rom_data = NULL;
	file_load_binary_fixed("runtime/minimal_6502/rom_test.bin", &ui_context.rom_data, 1 << 15);

	// create device
	ui_context.device = dev_minimal_6502_create(ui_context.rom_data);
}


void nuklear_gui(struct nk_context *ctx) {
	panel_memory_assembly(ctx, &ui_context);
}

void panel_memory_assembly(struct nk_context *nk_ctx, UIContext *ui_ctx) {
	static const char *panel_title = "ROM - assembler";
	static const struct nk_rect panel_bounds = {
		.x = 50,
		.y = 50,
		.w = 220,
		.h = 220
	};
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE;

	uint8_t *rom_data = ui_ctx->device->rom->data_array;

	if (nk_begin(nk_ctx, panel_title, panel_bounds, panel_flags)) {
		
		int index = 0;
		char *line = NULL;

		while (index < 0x8000) {
			index += filt_6502_asm_line(rom_data, 0x8000, index, 0x8000, &line);
			nk_layout_row_static(nk_ctx, 18, 200, 1);
			nk_label(nk_ctx, line, NK_TEXT_LEFT);
			arrsetlen(line, 0);
		}
	}
	nk_end(nk_ctx);

}
