// gui/panel_dev_commodore_pet.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_dev_commodore_pet.h"

#include <dev_commodore_pet.h>
#include <perif_pet_crt.h>

#include "ui_context.h"
#include "widgets.h"
#include "imgui_ex.h"

#include "panel_chip_6520.h"
#include "panel_chip_6522.h"
#include "panel_cpu_6502.h"
#include "panel_memory.h"
#include "panel_monitor.h"
#include "panel_input_pet.h"
#include "panel_display_rgba.h"
#include "panel_signals.h"
#include "panel_logic_analyzer.h"
#include "panel_datassette.h"
#include "panel_disk_2031.h"

class PanelDevCommodorePet : public Panel {
public:
	PanelDevCommodorePet(UIContext *ctx, ImVec2 pos, DevCommodorePet *dev) :
		Panel(ctx),
		position(pos),
		device(dev) {

		auto &cat_memory = pet_hardware.add_category("Memory");
		cat_memory.add_leaf("Main Ram")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_memory_create(ui_context, {220, 120}, ui_context->unique_panel_id("RAM").c_str(), 0x0000, 0x8000));
					});
		cat_memory.add_leaf("Video Ram")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_memory_create(ui_context, {220, 120}, ui_context->unique_panel_id("VRAM").c_str(), 0x8000, 40*25));
					});
		cat_memory.add_leaf("Basic Rom")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(), 0xb000, 0x3000));
					});
		cat_memory.add_leaf("Editor Rom")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(), 0xe000, 0x0800));
					});
		cat_memory.add_leaf("Kernal Rom")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(), 0xf000, 0x1000));
					});

		auto &cat_cpu = pet_hardware.add_category("CPU");
		cat_cpu.add_leaf("MOS Technology 6502")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_cpu_6502_create(ui_context, {220, 342}, device->cpu));
					});

		auto &cat_support = pet_hardware.add_category("Support Chips");
		cat_support.add_leaf("PIA (6520) - IEEE-488")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_chip_6520_create(ui_context, {420, 342}, device->pia_1));
					});

		cat_support.add_leaf("PIA (6520) - Keyboard")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_chip_6520_create(ui_context, {420, 342}, device->pia_2));
					});
		cat_support.add_leaf("VIA (6522)")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_chip_6522_create(ui_context, {420, 342}, device->via));
					});

		auto &cat_perif = pet_hardware.add_category("Peripherals");
		cat_perif.add_leaf("Datassette")
					.add_action("Open", [&]() {
						ui_context->panel_add(panel_datassette_create(ui_context, {340, 310}, device->datassette));
					});
		cat_perif.add_leaf("Floppy Disk")
					.add_action("Open", [&]() {
						ui_context->panel_add(panel_fd2031_create(ui_context, {340, 310}, device->disk_2031));
					});

		auto &cat_tools = pet_hardware.add_category("Tools");
		cat_tools.add_leaf("Monitor")
					.add_action("Open", [&]() {
						ui_context->panel_add(panel_monitor_create(ui_context, {340, 310}));
					});
		cat_tools.add_leaf("Logic Analyzer")
					.add_action("Open", [&]() {
						ui_context->panel_add(panel_logic_analyzer_create(ui_context, {340, 310}));
					});
		cat_tools.add_leaf("Signal Debugger")
					.add_action("Open", [&]() {
						ui_context->panel_add(panel_signals_create(ui_context, {340, 310}));
					});

		diag_mode = device->diag_mode;
	}

	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title)) {

			pet_hardware.display();

			if (ImGui::CollapsingHeader("Advanced")) {
				if (ImGui::Checkbox("Assert Diagnostice Sense Line", &diag_mode)) {
					dev_commodore_pet_diag_mode(device, diag_mode);
				}
			}
		}

		ImGui::End();
	}

private:
	ImVec2			position;
	const ImVec2	size = {0, 0};
	DevCommodorePet *device;

	UITree			pet_hardware;
	bool			diag_mode = false;

	constexpr static const char *title = "Device - Commodore PET 2001N";
};

Panel::uptr_t panel_dev_commodore_pet_create(UIContext *ctx, ImVec2 pos, DevCommodorePet *device) {

	// a keyboard panel is always useful
	auto keyboard_pnl = panel_input_pet_create(ctx, {340, 512}, device->keypad);
	ctx->panel_add(std::move(keyboard_pnl));

	// display panel
	auto display_pnl = panel_display_rgba_create(ctx, {340, 10}, device->screen);
	ctx->panel_add(std::move(display_pnl));

	// create panel for the commodore pet
	return std::make_unique<PanelDevCommodorePet>(ctx, pos, device);
}
