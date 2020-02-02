// gui/main_gui.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include <imgui/imgui.h>

#include "dev_minimal_6502.h"
#include "filt_6502_asm.h"

#include "panel_clock.h"
#include "panel_cpu_6502.h"
#include "panel_memory.h"
#include "panel_control.h"
#include "panel_monitor.h"
#include "panel_chip_6520.h"
#include "panel_chip_hd44780.h"

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
	ui_context.panels.push_back(
			panel_control_create(&ui_context, {2, 10}, "runtime/minimal_6502"));

	ui_context.panels.push_back(
			panel_memory_create(&ui_context, {2, 120}, "RAM",
								ui_context.device->ram->data_array, 0x8000, 0x0000));

	ui_context.panels.push_back(
			panel_memory_create(&ui_context, {442, 120}, "ROM",
								ui_context.device->rom->data_array, 0x4000, 0xC000));

	ui_context.panels.push_back(
			panel_cpu_6502_create(&ui_context, {2, 342}, ui_context.device->cpu));

	ui_context.panels.push_back(
			panel_chip_6520_create(&ui_context, {420, 342}, ui_context.device->pia));

	ui_context.panels.push_back(
			panel_clock_create(&ui_context, {370, 10}, ui_context.device->clock));

	ui_context.panels.push_back(
			panel_monitor_create(&ui_context, {890, 180}));

	ui_context.panels.push_back(
			panel_chip_hd44780_create(&ui_context, {740, 10}, ui_context.device->lcd));

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

	for (auto &panel : ui_context.panels) {
		panel->display();
	}
}

