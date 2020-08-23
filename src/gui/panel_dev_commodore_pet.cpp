// gui/panel_dev_commodore_pet.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_dev_commodore_pet.h"

#include "dev_commodore_pet.h"
#include "display_pet_crt.h"

#include "ui_context.h"
#include "widgets.h"

#include "panel_chip_6520.h"
#include "panel_chip_6522.h"
#include "panel_cpu_6502.h"
#include "panel_memory.h"
#include "panel_monitor.h"
#include "panel_input_pet.h"
#include "panel_display_rgba.h"
#include "panel_signals.h"

#include "popup_file_selector.h"


namespace {

static std::string binary_path = "runtime/commodore_pet/prg";

static inline std::string path_for_binary(const std::string & filename) {
	std::string result = binary_path;
	result += "/";
	result += filename;

	return result;
}

} // unnamed namespace

class PanelDevCommodorePet : public Panel {
public:

	PanelDevCommodorePet(UIContext *ctx, ImVec2 pos, DevCommodorePet *device) :
		Panel(ctx),
		position(pos),
		device(device) {
	}

	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		bool load_prg = false;

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

				/*
				ImGui::Text("Video RAM");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_vram")) {
					auto pnl = panel_memory_create(ui_context, {2, 120}, ui_context->unique_panel_id("VRAM").c_str(),
												   device->vram->data_array, 40*25, 0x8000);
					ui_context->panel_add(std::move(pnl));
				}
				*/

				ImGui::Text("Basic ROM (0xb000");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_basic_rom_b")) {
					auto pnl = panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(),
												   device->roms[0]->data_array, 0x1000, 0xB000);
					ui_context->panel_add(std::move(pnl));
				}

				ImGui::Text("Basic ROM (0xc000");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_basic_rom_c")) {
					auto pnl = panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(),
												   device->roms[1]->data_array, 0x1000, 0xC000);
					ui_context->panel_add(std::move(pnl));
				}

				ImGui::Text("Basic ROM (0xd000");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_basic_rom_d")) {
					auto pnl = panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(),
												   device->roms[2]->data_array, 0x1000, 0xD000);
					ui_context->panel_add(std::move(pnl));
				}

				ImGui::Text("Editor ROM (0xe000");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_editor_rom")) {
					auto pnl = panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(),
												   device->roms[3]->data_array, 0x0800, 0xE000);
					ui_context->panel_add(std::move(pnl));
				}

				ImGui::Text("Kernal ROM (0xf000");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_kernal_rom")) {
					auto pnl = panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(),
												   device->roms[4]->data_array, 0x1000, 0xF000);
					ui_context->panel_add(std::move(pnl));
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
				ImGui::Text("PIA - 6520 (IEEE-488)");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_pia_1")) {
					auto pnl = panel_chip_6520_create(ui_context, {420, 342}, device->pia_1);
					ui_context->panel_add(std::move(pnl));
				}

				ImGui::Text("PIA - 6520 (Keyboard)");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_pia_2")) {
					auto pnl = panel_chip_6520_create(ui_context, {420, 342}, device->pia_2);
					ui_context->panel_add(std::move(pnl));
				}

				ImGui::Text("VIA - 6522");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_via")) {
					auto pnl = panel_chip_6522_create(ui_context, {420, 342}, device->via);
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

				ImGui::Text("Signals");
				ImGui::SameLine();
				if (ImGui::SmallButton("Open##open_signals")) {
					auto pnl = panel_signals_create(ui_context, {340, 310});
					ui_context->panel_add(std::move(pnl));
				}

				ImGui::Text("Prg-file");
				ImGui::SameLine();
				load_prg = ImGui::SmallButton("Load##load_prg");
				if (!prg_last_loaded.empty()) {
					ImGui::Text(" - last loaded: %s", prg_last_loaded.c_str());
				}

				ImGui::TreePop();
			}
		}

		prg_selection->define_popup();
		if (load_prg) {
			prg_selection->display_popup([&](std::string selected_file) {
				dev_commodore_pet_load_prg(device, path_for_binary(selected_file).c_str(), true);
				prg_last_loaded = selected_file;
			});
		}

		ImGui::End();
	}

private:
	ImVec2			position;
	const ImVec2	size = {330, 320};
	DevCommodorePet *device;

	std::string		prg_last_loaded = "";

	PopupFileSelector::uptr_t prg_selection = PopupFileSelector::make_unique(ui_context, binary_path.c_str() , ".prg", "");

	constexpr static const char *title = "Device - Commodore PET 2001N";
};

Panel::uptr_t panel_dev_commodore_pet_create(UIContext *ctx, ImVec2 pos, DevCommodorePet *device) {

	// a keyboard panel is always useful
	auto keyboard_pnl = panel_input_pet_create(ctx, {340, 400}, device->keypad);
	ctx->panel_add(std::move(keyboard_pnl));

	// display panel
	auto display_pnl = panel_display_rgba_create(ctx, {340, 20}, device->crt->display);
	ctx->panel_add(std::move(display_pnl));

	// create panel for the commodore pet
	return std::make_unique<PanelDevCommodorePet>(ctx, pos, device);
}
