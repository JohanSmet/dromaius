// gui/main_gui.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "window_main.h"
#include <imgui/imgui.h>

#include "dev_minimal_6502.h"
#include "dev_commodore_pet.h"
#include "filt_6502_asm.h"

#include "panel_control.h"
#include "panel_dev_minimal_6502.h"
#include "panel_dev_commodore_pet.h"
#include "panel_memory.h"

#include "context.h"
#include "cpu.h"

namespace {
	enum {
		MACHINE_COMMODORE_PET = 0,
		MACHINE_COMMODORE_PET_LITE = 1,
		MACHINE_MINIMAL_6502 = 2
	};

	constexpr static const char *MACHINE_NAMES[] = {
		"Commodore PET - 2001N",
		"Commodore PET - 2001N (lite)",
		"Minimal 6502"
	};

} // unnamed namespace

void WindowMain::on_start() {
	create_device(0);
	panel_memory_load_fonts();
}

void WindowMain::on_render_frame() {

#ifdef DMS_NO_THREADING
	dms_execute(ui_context.dms_ctx);
#endif // DMS_NO_THREADING

	Cpu *cpu = ui_context.device->get_cpu(ui_context.device);

	if (cpu->is_at_start_of_instruction(cpu)) {
		ui_context.last_pc = cpu->program_counter(cpu);
	}

	panel_select_machine();

	ui_context.panel_foreach([&](auto panel) {
		if (!panel) return;
		panel->display();

		if (panel->want_close()) {
			ui_context.panel_close(panel);
		}
	});
}

void WindowMain::create_device(int index) {

	// create device
	switch (index) {
		case MACHINE_COMMODORE_PET:
			ui_context.device = create_commodore_pet(false);
			break;
		case MACHINE_COMMODORE_PET_LITE:
			ui_context.device = create_commodore_pet(true);
			break;
		case MACHINE_MINIMAL_6502:
			ui_context.device = create_minimal_6502();
			break;
	}
	ui_context.last_pc = 0;

#ifndef DMS_NO_THREADING
	// start dromaius context
	dms_start_execution(ui_context.dms_ctx);
#endif // DMS_NO_THREADING
}

Device *WindowMain::create_minimal_6502() {

	DevMinimal6502 *device = dev_minimal_6502_create(NULL);

	// create dromaius context
	ui_context.dms_ctx = dms_create_context();
	dms_set_device(ui_context.dms_ctx, (Device *) device);

	// create UI panels
	ui_context.panel_add(panel_control_create(&ui_context, {0, 55}, device->oscillator,
											  {device->signals[SIG_M6502_CPU_SYNC], true, false},
											  {{device->signals[SIG_M6502_CLOCK], true, true}}
	));

	ui_context.panel_add(
			panel_dev_minimal_6502_create(&ui_context, {0, 110}, device));

	return (Device *) device;
}

Device *WindowMain::create_commodore_pet(bool lite) {

	DevCommodorePet *device = (lite) ? dev_commodore_pet_lite_create() : dev_commodore_pet_create();

	// create dromaius context
	ui_context.dms_ctx = dms_create_context();
	dms_set_device(ui_context.dms_ctx, (Device *) device);

	// create UI panels
	ui_context.panel_add(panel_control_create(&ui_context, {0, 55}, device->oscillator_y1,
											  {device->signals[SIG_P2001N_SYNC], true, false},
											  {{device->signals[SIG_P2001N_CLK1], true, true},
											   {device->signals[SIG_P2001N_CLK8], true, true},
											   {device->signals[SIG_P2001N_CLK16], true, true}}
	));

	ui_context.panel_add(
			panel_dev_commodore_pet_create(&ui_context, {0, 110}, device));

	return (Device *) device;
}

void WindowMain::panel_select_machine() {

	ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(330,0), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Emulated machine")) {
		if (ImGui::Combo("Machine", &active_machine, MACHINE_NAMES, sizeof(MACHINE_NAMES) / sizeof(MACHINE_NAMES[0]))) {
			switch_machine(active_machine);
		}
	}
	ImGui::End();
}

void WindowMain::switch_machine(int index) {

#ifndef DMS_NO_THREADING
	dms_stop_execution(ui_context.dms_ctx);
#endif

	// close panels
	ui_context.panel_close_all();

	// release device
	auto *device = dms_get_device(ui_context.dms_ctx);
	device->destroy(device);

	// release context
	dms_release_context(ui_context.dms_ctx);

	// create new device
	create_device(index);
}
