// gui/main_gui.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include <nuklear/nuklear_std.h>

#include "dev_minimal_6502.h"
#include "filt_6502_asm.h"

#include "panel_clock.h"
#include "panel_cpu_6502.h"
#include "panel_memory.h"
#include "panel_control.h"
#include "panel_monitor.h"

#include "ui_context.h"
#include "context.h"

#include <stdlib.h>

UIContext ui_context;

char *nuklear_config_window_title(void) {
	return "Dromaius";
}

void nuklear_on_start(struct nk_context *ctx) {

	// create device
	ui_context.device = dev_minimal_6502_create(NULL);
	ui_context.device->line_reset_b = ACTLO_DEASSERT;
	ui_context.last_pc = 0;

	// create dromaius context
	ui_context.dms_ctx = dms_create_context();
	dms_set_device(ui_context.dms_ctx, ui_context.device);

	// create UI panels
	ui_context.pnl_ram = panel_memory_init(
							ctx, (struct nk_vec2) {300, 100}, "RAM",
							ui_context.device->ram->data_array, 0x8000, 0x0000);
	ui_context.pnl_rom = panel_memory_init(
							ctx, (struct nk_vec2) {750, 100}, "ROM",
							ui_context.device->rom->data_array, 0x8000, 0x8000);

	ui_context.pnl_control = panel_control_init(ctx, nk_vec2(20, 20), ui_context.dms_ctx, "runtime/minimal_6502");

	ui_context.pnl_monitor = panel_monitor_init(ctx, nk_vec2(750, 400), ui_context.dms_ctx);

	// load default rom
	panel_control_select_rom(ui_context.pnl_control, 0);

	// reset device
	dev_minimal_6502_reset(ui_context.device);
	
	// start dromaius context
	dms_start_execution(ui_context.dms_ctx);
}

void nuklear_gui(struct nk_context *ctx) {
	if (ui_context.device->line_cpu_sync) {
		ui_context.last_pc = ui_context.device->cpu->reg_pc;
	}

	panel_control_display(ui_context.pnl_control);
	panel_cpu_6502(ctx, (struct nk_vec2) {300, 400}, ui_context.device->cpu);
	panel_memory_display(ui_context.pnl_ram, ui_context.last_pc);
	panel_memory_display(ui_context.pnl_rom, ui_context.last_pc);
	panel_clock(ctx, ui_context.device->clock, (struct nk_vec2) {20, 400});
	panel_monitor_display(ui_context.pnl_monitor);
}

