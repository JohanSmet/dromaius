// gui/main_gui.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include <nuklear/nuklear_std.h>

#include "dev_minimal_6502.h"
#include "filt_6502_asm.h"

#include "panel_cpu_6502.h"
#include "panel_memory_raw.h"
#include "panel_memory_disasm.h"
#include "panel_control.h"

#include "ui_context.h"

#include "utils.h"
#include <stb/stb_ds.h>

UIContext ui_context;

void panel_control(struct nk_context *nk_ctx, UIContext *ui_ctx, struct nk_vec2 pos);

char *nuklear_config_window_title(void) {
	return "Dromaius";
}

void nuklear_on_start(struct nk_context *ctx) {
	// load ROM
	uint8_t *rom_data = NULL;
	file_load_binary_fixed("runtime/minimal_6502/rom_test.bin", &rom_data, 1 << 15);

	// create device
	ui_context.device = dev_minimal_6502_create(rom_data);
	ui_context.device->line_reset_b = ACTLO_DEASSERT;

	arrfree(rom_data);
}

void nuklear_gui(struct nk_context *ctx) {
	panel_control(ctx, &ui_context, (struct nk_vec2) {20,20});
	panel_memory_disasm(ctx, &ui_context, (struct nk_vec2) {20, 100}, "ROM - DisAsm", ui_context.device->rom->data_array, 0x8000, 0x8000);
	panel_memory_raw(ctx, (struct nk_vec2) {300, 100}, "ROM", ui_context.device->rom->data_array, 0x8000, 0x8000);
	panel_memory_raw(ctx, (struct nk_vec2) {750, 100}, "RAM", ui_context.device->ram->data_array, 0x8000, 0x0000);
	panel_cpu_6502(ctx, (struct nk_vec2) {300, 400}, ui_context.device->cpu);
}
