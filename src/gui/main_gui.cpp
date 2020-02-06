// gui/main_gui.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include <imgui/imgui.h>

#include "dev_minimal_6502.h"
#include "filt_6502_asm.h"

#include "panel_clock.h"
#include "panel_control.h"
#include "panel_dev_minimal_6502.h"

#include "ui_context.h"
#include "context.h"

UIContext ui_context;

const char *ui_config_window_title() {
	return "Dromaius";
}

void ui_on_start() {

	// create device
	ui_context.device = dev_minimal_6502_create(NULL);
	//ui_context.device->line_reset_b = ACTLO_DEASSERT;
	ui_context.last_pc = 0;

	// create dromaius context
	ui_context.dms_ctx = dms_create_context();
	dms_set_device(ui_context.dms_ctx, ui_context.device);

	// create UI panels
	ui_context.panel_add(panel_control_create(&ui_context, {2, 10}));

	ui_context.panel_add(
			panel_clock_create(&ui_context, {370, 10}, ui_context.device->clock));

	ui_context.panel_add(
			panel_dev_minimal_6502_create(&ui_context, {10, 500}, ui_context.device));

	// reset device
	dev_minimal_6502_reset(ui_context.device);

#ifndef DMS_NO_THREADING
	// start dromaius context
	dms_start_execution(ui_context.dms_ctx);
#endif // DMS_NO_THREADING

}

void ui_frame() {

#ifdef DMS_NO_THREADING
	dms_execute(ui_context.dms_ctx);
#endif // DMS_NO_THREADING

	if (cpu_6502_at_start_of_instruction(ui_context.device->cpu)) {
		ui_context.last_pc = ui_context.device->cpu->reg_pc;
	}

	ui_context.panel_foreach([](auto panel) {
		if (!panel) return;
		panel->display();

		if (panel->want_close()) {
			ui_context.panel_close(panel);
		}
	});
}

