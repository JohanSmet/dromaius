// gui/main_gui.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include <imgui/imgui.h>

#include "dev_minimal_6502.h"
#include "dev_commodore_pet.h"
#include "filt_6502_asm.h"

#include "panel_clock.h"
#include "panel_control.h"
#include "panel_dev_minimal_6502.h"
#include "panel_dev_commodore_pet.h"
#include "panel_memory.h"

#include "ui_context.h"
#include "context.h"

UIContext ui_context;

const char *ui_config_window_title() {
	return "Dromaius";
}

Device *create_minimal_6502(UIContext *ui_context) {

	DevMinimal6502 *device = dev_minimal_6502_create(NULL);

	// create dromaius context
	ui_context->dms_ctx = dms_create_context();
	dms_set_device(ui_context->dms_ctx, (Device *) device);

	// create UI panels
	ui_context->panel_add(panel_control_create(ui_context, {0, 0}));

	ui_context->panel_add(
			panel_clock_create(ui_context, {0, 310}, device->clock));

	ui_context->panel_add(
			panel_dev_minimal_6502_create(ui_context, {0, 55}, device));

	// reset device
	device->reset(device);

	return (Device *) device;
}

Device *create_commodore_pet(UIContext *ui_context) {

	DevCommodorePet *device = dev_commodore_pet_create();

	// create dromaius context
	ui_context->dms_ctx = dms_create_context();
	dms_set_device(ui_context->dms_ctx, (Device *) device);

	// create UI panels
	ui_context->panel_add(panel_control_create(ui_context, {0, 0}));

	ui_context->panel_add(
			panel_clock_create(ui_context, {0, 310}, device->clock));

	ui_context->panel_add(
			panel_dev_commodore_pet_create(ui_context, {0, 55}, device));

	// reset device
	device->reset(device);

	return (Device *) device;
}

void ui_on_start() {
	// create device
	// ui_context.device = create_minimal_6502(&ui_context);
	ui_context.device = create_commodore_pet(&ui_context);
	ui_context.last_pc = 0;

#ifndef DMS_NO_THREADING
	// start dromaius context
	dms_start_execution(ui_context.dms_ctx);
#endif // DMS_NO_THREADING

	panel_memory_load_fonts();
}

void ui_frame() {

#ifdef DMS_NO_THREADING
	dms_execute(ui_context.dms_ctx);
#endif // DMS_NO_THREADING

	Cpu *cpu = ui_context.device->get_cpu(ui_context.device);

	if (cpu->is_at_start_of_instruction(cpu)) {
		ui_context.last_pc = cpu->program_counter(cpu);
	}

	ui_context.panel_foreach([](auto panel) {
		if (!panel) return;
		panel->display();

		if (panel->want_close()) {
			ui_context.panel_close(panel);
		}
	});
}

