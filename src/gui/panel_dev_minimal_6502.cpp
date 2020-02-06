// gui/panel_dev_minimal_6502.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_dev_minimal_6502.h"

#include "dev_minimal_6502.h"
#include "ui_context.h"
#include "widgets.h"

#include "panel_chip_6520.h"
#include "panel_chip_hd44780.h"
#include "panel_cpu_6502.h"
#include "panel_memory.h"
#include "panel_monitor.h"

#include "popup_file_selector.h"

namespace {

static std::string binary_path = "runtime/minimal_6502";

static inline std::string path_for_binary(const std::string & filename) {
	std::string result = binary_path;
	result += "/";
	result += filename;

	return result;
}


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
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		bool load_rom = false;
		bool load_ram = false;

		if (ImGui::Begin(title)) {

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Memory")) {
				ImGui::Text("RAM (32k)");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_ram")) {
					auto pnl = panel_memory_create(ui_context, {2, 120}, ui_context->unique_panel_id("RAM").c_str(),
												   device->ram->data_array, 0x8000, 0x0000);
					ui_context->panel_add(std::move(pnl));
				}
				ImGui::SameLine();
				load_ram = ImGui::SmallButton("Load##load_ram");

				ImGui::Text("ROM (32k)");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_rom")) {
					auto pnl = panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(),
												   device->rom->data_array, 0x4000, 0xC000);
					ui_context->panel_add(std::move(pnl));
				}
				ImGui::SameLine();
				load_rom = ImGui::SmallButton("Load##load_rom");

				if (!rom_last_loaded.empty()) {
					ImGui::Text(" - last loaded: %s", rom_last_loaded.c_str());
				}

				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("CPU")) {
				ImGui::Text("MOS Technology 6502");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_cpu")) {
					auto pnl = panel_cpu_6502_create(ui_context, {2, 342}, device->cpu);
					ui_context->panel_add(std::move(pnl));
				}
				
				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Support Chips")) {
				ImGui::Text("PIA - MOS Technology 6520");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_pia")) {
					auto pnl = panel_chip_6520_create(ui_context, {420, 342}, device->pia);
					ui_context->panel_add(std::move(pnl));
				}

				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Tools")) {
				ImGui::Text("Monitor");
				ImGui::SameLine();
				if (ImGui::SmallButton("Open##open_monitor")) {
					auto pnl = panel_monitor_create(ui_context, {340, 310});
					ui_context->panel_add(std::move(pnl));
				}
				ImGui::TreePop();
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
	const ImVec2	size = {330, 250};
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
	
	// create panel for the minimal_6502
	return std::make_unique<PanelDevMinimal6502>(ctx, pos, device);
}
