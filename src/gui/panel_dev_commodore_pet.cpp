// gui/panel_dev_commodore_pet.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_dev_commodore_pet.h"

#include "dev_commodore_pet.h"
#include "ui_context.h"
#include "widgets.h"

#include "panel_chip_6520.h"
#include "panel_cpu_6502.h"
#include "panel_memory.h"
#include "panel_monitor.h"

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

				ImGui::Text("Video RAM");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_vram")) {
					auto pnl = panel_memory_create(ui_context, {2, 120}, ui_context->unique_panel_id("VRAM").c_str(),
												   device->vram->data_array, 40*25, 0x8000);
					ui_context->panel_add(std::move(pnl));
				}


				/*ImGui::Text("Basic ROM");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_basic_rom")) {
					auto pnl = panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(),
												   device->rom->data_array, 0x4000, 0xC000);
					ui_context->panel_add(std::move(pnl));
				}*/

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

				ImGui::Text("VIA - 6520 (Keyboard)");
				ImGui::SameLine();
				if (ImGui::SmallButton("View##view_pia_2")) {
					auto pnl = panel_chip_6520_create(ui_context, {420, 342}, device->pia_2);
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
	}

private:
	ImVec2			position;
	const ImVec2	size = {330, 250};
	DevCommodorePet *device;

	constexpr static const char *title = "Device - Commodore PET 2001N";
};

Panel::uptr_t panel_dev_commodore_pet_create(UIContext *ctx, ImVec2 pos, DevCommodorePet *device) {

	// a keypad panel is always useful
	//auto keypad_pnl = panel_input_keypad_create(ctx, {340, 200}, device->keypad);
	//ctx->panel_add(std::move(keypad_pnl));

	// create panel for the commodore pet
	return std::make_unique<PanelDevCommodorePet>(ctx, pos, device);
}
