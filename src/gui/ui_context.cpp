// gui/ui_context.h - Johan Smet - BSD-3-Clause (see LICENSE)

#include "ui_context.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <context.h>
#include <cpu.h>
#include <dev_minimal_6502.h>
#include <dev_commodore_pet.h>

#include "panel_control.h"
#include "panel_dev_commodore_pet.h"
#include "panel_dev_minimal_6502.h"
#include "panel_memory.h"

namespace {


} // unnamed namespace

void UIContext::switch_machine(MachineType machine) {

#ifndef DMS_NO_THREADING
	if (dms_ctx) {
		dms_stop_execution(dms_ctx);
	}
#endif

	// close panels
	panel_close_all();

	if (dms_ctx) {
		// release device
		auto *device = dms_get_device(dms_ctx);
		if (device) {
			device->destroy(device);
			device = nullptr;
		}

		// release context
		dms_release_context(dms_ctx);
	}

	// create new device
	create_device(machine);
}

void UIContext::setup_ui(struct GLFWwindow *window) {
	glfw_window = window;
	switch_machine(config.machine_type);
	panel_memory_load_fonts();
}

void UIContext::draw_ui() {

#ifdef DMS_NO_THREADING
	dms_execute(dms_ctx);
#endif // DMS_NO_THREADING

	Cpu *cpu = device->get_cpu(device);

	if (cpu && cpu->is_at_start_of_instruction(cpu)) {
		last_pc = cpu->program_counter(cpu);
	}

	// setup_dockspace();

	for (auto &panel : panels) {
		if (!panel) return;
		panel->display_panel();

		if (panel->want_close()) {
			panel_close(panel.get());
		}
	}
}

void UIContext::panel_add(Panel::uptr_t panel) {
	panels.push_back(std::move(panel));
}

void UIContext::panel_close(Panel *panel) {
	dms::remove_owner(panels, panel);
}

void UIContext::panel_close_all() {
	panels.clear();
}

void UIContext::create_device(MachineType machine) {

	// create device
	switch (machine) {
		case MachineType::CommodorePet:
			create_commodore_pet(false);
			break;
		case MachineType::CommodorePetLite:
			create_commodore_pet(true);
			break;
		case MachineType::Minimal6502:
			create_minimal_6502();
			break;
	}
	last_pc = 0;

#ifndef DMS_NO_THREADING
	// start dromaius context
	dms_start_execution(dms_ctx);
#endif // DMS_NO_THREADING
}

void UIContext::create_minimal_6502() {

	DevMinimal6502 *device_6502 = dev_minimal_6502_create(NULL);
	device = (Device *) device_6502;

	// create dromaius context
	dms_ctx = dms_create_context();
	dms_set_device(dms_ctx, device);

	// create UI panels
	panel_add(panel_control_create(this, {0, 55}, device_6502->oscillator,
								  {device_6502->signals[SIG_M6502_CPU_SYNC], true, false},
								  {{device_6502->signals[SIG_M6502_CLOCK], true, true}}
	));

	panel_add(panel_dev_minimal_6502_create(this, {0, 110}, device_6502));
}

void UIContext::create_commodore_pet(bool lite) {

	DevCommodorePet *device_pet = (lite) ? dev_commodore_pet_lite_create() : dev_commodore_pet_create();
	device = (Device *) device_pet;

	// create dromaius context
	dms_ctx = dms_create_context();
	dms_set_device(dms_ctx, device);

	// create UI panels
	panel_add(panel_control_create(this, {0, 55}, device_pet->oscillator_y1,
								  {device_pet->signals[SIG_P2001N_SYNC], true, false},
								  {{device_pet->signals[SIG_P2001N_CLK1], true, true},
								   {device_pet->signals[SIG_P2001N_CLK8], true, true},
								   {device_pet->signals[SIG_P2001N_CLK16], true, true}}
	));

	panel_add(panel_dev_commodore_pet_create(this, {0, 110}, device_pet));
}

void UIContext::setup_dockspace() {

	auto viewport = ImGui::GetMainViewport();
	dock_id_main = ImGui::DockSpaceOverViewport(viewport, ImGuiDockNodeFlags_PassthruCentralNode);

	if (dock_id_left_top == 0) {
		ImGui::DockBuilderRemoveNode(dock_id_main);			// Clear out existing layout
		ImGui::DockBuilderAddNode(dock_id_main, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dock_id_main, viewport->Size);

		dock_id_left_top = ImGui::DockBuilderSplitNode(	dock_id_main, ImGuiDir_Left, 0.25f,
														NULL, &dock_id_main);
		dock_id_left_mid = ImGui::DockBuilderSplitNode(	dock_id_left_top, ImGuiDir_Down, 0.80f,
														NULL, &dock_id_left_top);
		dock_id_left_bot = ImGui::DockBuilderSplitNode(	dock_id_left_mid, ImGuiDir_Down, 0.80f,
														NULL, &dock_id_left_mid);

		ImGui::DockBuilderSetNodeSize(dock_id_left_top, {viewport->Size.x * 0.25f, 50 });

		ImGui::DockBuilderFinish(dock_id_main);
	}
}