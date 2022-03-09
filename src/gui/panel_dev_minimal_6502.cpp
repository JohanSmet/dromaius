// gui/panel_dev_minimal_6502.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_dev_minimal_6502.h"

#include <dev_minimal_6502.h>
#include "ui_context.h"
#include "widgets.h"

#include "panel_chip_6520.h"
#include "panel_chip_hd44780.h"
#include "panel_cpu_6502.h"
#include "panel_input_keypad.h"
#include "panel_memory.h"
#include "panel_monitor.h"
#include "panel_signals.h"

#include "popup_file_selector.h"

namespace {

static std::string binary_path = "runtime/minimal_6502";

static inline std::string path_for_binary(const std::string & filename) {
	std::string result = binary_path;
	result += "/";
	result += filename;

	return result;
}

static constexpr int32_t BUTTON_RAM_LOAD = 1;
static constexpr int32_t BUTTON_ROM_LOAD = 2;

static const UITree<DevMinimal6502>::data_t MINIMAL_6502_HARDWARE = {
	UI_TREE_NODE("Memory")
		UI_TREE_LEAF("RAM (32k)")
			UI_TREE_ACTION_PANEL("View")
				return	panel_memory_create(ctx, {2, 120}, ctx->unique_panel_id("RAM").c_str(), 0x0000, 0x8000);
			UI_TREE_ACTION_END,
			UI_TREE_ACTION_CALLBACK("Load", BUTTON_RAM_LOAD)
		UI_TREE_LEAF_END,

		UI_TREE_LEAF("ROM (16k)")
			UI_TREE_ACTION_PANEL("View")
				return panel_memory_create(ctx, {442, 120}, ctx->unique_panel_id("ROM").c_str(), 0xc000, 0x4000);
			UI_TREE_ACTION_END,
			UI_TREE_ACTION_CALLBACK("Load", BUTTON_ROM_LOAD)
		UI_TREE_LEAF_END,
	UI_TREE_NODE_END,

	UI_TREE_NODE("CPU")
		UI_TREE_LEAF("MOS Technology 6502")
			UI_TREE_ACTION_PANEL("View")
				return panel_cpu_6502_create(ctx, {2, 342}, dev->cpu);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
	UI_TREE_NODE_END,

	UI_TREE_NODE("Support Chips")
		UI_TREE_LEAF("PIA (6520)")
			UI_TREE_ACTION_PANEL("View")
				return panel_chip_6520_create(ctx, {420, 342}, dev->pia);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
	UI_TREE_NODE_END,

	UI_TREE_NODE("Tools")
		UI_TREE_LEAF("Monitor")
			UI_TREE_ACTION_PANEL("Open")
				return panel_monitor_create(ctx, {340, 310});
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
		UI_TREE_LEAF("Signal Debugger")
			UI_TREE_ACTION_PANEL("Open")
				return panel_signals_create(ctx, {340, 310});
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
	UI_TREE_NODE_END,
};

} // unnamed namespace

class PanelDevMinimal6502 : public Panel {
public:

	PanelDevMinimal6502(UIContext *ctx, ImVec2 pos, DevMinimal6502 *device) :
		Panel(ctx),
		position(pos),
		device(device) {
		// load default rom image
		auto roms = rom_selection->construct_directory_listing();
		if (!roms.empty()) {
			rom_last_loaded = roms[0];
			dev_minimal_6502_rom_from_file(device, path_for_binary(roms[0]).c_str());
		}
	}

	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);

		bool load_rom = false;
		bool load_ram = false;

		if (ImGui::Begin(title)) {

			UITree<DevMinimal6502>::display(
					MINIMAL_6502_HARDWARE, ui_context, device,
					[&](int button) {
						switch (button) {
							case BUTTON_RAM_LOAD:
								load_ram = true;
								break;
							case BUTTON_ROM_LOAD:
								load_rom = true;
								break;
						}
					}
			);

			if (!rom_last_loaded.empty() || !ram_last_loaded.empty()) {
				ImGui::Spacing();
				ImGui::Separator();
				if (!rom_last_loaded.empty()) {
					ImGui::Text("Loaded ROM: %s", rom_last_loaded.c_str());
				}
				if (!ram_last_loaded.empty()) {
					ImGui::Text("Loaded RAM: %s", ram_last_loaded.c_str());
				}
			}
		}

		ImGui::End();

		rom_selection->define_popup();
		if (load_rom) {
			rom_selection->display_popup([&](std::string selected_file) {
				dev_minimal_6502_rom_from_file(device, path_for_binary(selected_file).c_str());
				rom_last_loaded = selected_file;
			});
		}

		ram_selection->define_popup();
		if (load_ram) {
			ram_selection->display_popup([&](std::string selected_file) {
				dev_minimal_6502_ram_from_file(device, path_for_binary(selected_file).c_str());
				ram_last_loaded = selected_file;
			});
		}
	}

private:
	ImVec2			position;
	DevMinimal6502 *device;

	std::string		rom_last_loaded = "";
	std::string		ram_last_loaded = "";

	PopupFileSelector::uptr_t rom_selection = PopupFileSelector::make_unique(ui_context, "runtime/minimal_6502", ".bin", "rom_");
	PopupFileSelector::uptr_t ram_selection = PopupFileSelector::make_unique(ui_context, "runtime/minimal_6502", ".bin", "ram_");

	constexpr static const char *title = "Device - Minimal 6502";
};

Panel::uptr_t panel_dev_minimal_6502_create(UIContext *ctx, ImVec2 pos, DevMinimal6502 *device) {

	// always create an output panel
	auto lcd_pnl = panel_chip_hd44780_create(ctx, {340, 0}, device->lcd);
	ctx->panel_add(std::move(lcd_pnl));

	// a keypad panel is always useful
	auto keypad_pnl = panel_input_keypad_create(ctx, {340, 200}, device->keypad);
	ctx->panel_add(std::move(keypad_pnl));

	// create panel for the minimal_6502
	return std::make_unique<PanelDevMinimal6502>(ctx, pos, device);
}
