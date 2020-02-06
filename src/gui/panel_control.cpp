// gui/panel_control.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to control the emulator

#include "panel_control.h"
#include "ui_context.h"
#include "context.h"

#include <imgui.h>

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

class PanelControl : public Panel {
public:
	PanelControl(UIContext *ctx, ImVec2 pos) :
		Panel(ctx),
		position(pos) {
	}

	void display() override {
		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		ImGui::Begin(title);

			if (ImGui::Button("Single")) {
				dms_single_step(ui_context->dms_ctx);
			}
			ImGui::SameLine();

			if (ImGui::Button("Step")) {
				dms_single_instruction(ui_context->dms_ctx);
			}
			ImGui::SameLine();

			if (ImGui::Button("Run")) {
				dms_run(ui_context->dms_ctx);
			}
			ImGui::SameLine();

			if (ImGui::Button("Pause")) {
				dms_pause(ui_context->dms_ctx);
			}
			ImGui::SameLine();

			if (ImGui::Button("Reset")) {
				dms_reset(ui_context->dms_ctx);
			}

		ImGui::End();
	}

private:
	ImVec2					position;
	const ImVec2			size = {360, 0};

	static constexpr const char *title = "Emulator Control";
};

Panel::uptr_t panel_control_create(UIContext *ctx, ImVec2 pos) {
	return std::make_unique<PanelControl>(ctx, pos);
}
