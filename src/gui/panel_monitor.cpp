// gui/panel_monitor.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// a simple hardware monitor

#include "panel_monitor.h"
#include "context.h"
#include "ui_context.h"

#include "imgui_ex.h"
#include <vector>

class PanelMonitor : public Panel {
public:
	PanelMonitor(UIContext *ctx, ImVec2 pos) :
		Panel(ctx),
		position(pos) {
		input[0] = '\0';
		title = ui_context->unique_panel_id("Monitor");
	}

	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {

			// previous input + output
			auto region = ImGui::GetContentRegionAvail();
			region.y -= 20;
			if (ImGui::BeginChild("output", region, false, 0)) {

				for (const auto &line : output) {
					ImGui::TextUnformatted(line.c_str());
				}

				ImGui::SetScrollHereY(1.0f);

				ImGui::EndChild();
			}

			// input
			ImGui::SetNextItemWidth(region.x * 0.8f);

			if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
				ImGui::SetKeyboardFocusHere(0);
			}

			if (ImGui::InputText("##input", input, sizeof(input) - 1, ImGuiInputTextFlags_EnterReturnsTrue)) {
				output.push_back(input);

				char *reply = NULL;
				dms_monitor_cmd(ui_context->dms_ctx, input, &reply);
				output.push_back(reply);

				input[0] = '\0';

				ImGui::SetItemDefaultFocus();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear")) {
				output.clear();
			}
		}
		ImGui::End();
	}

private:
	ImVec2			position;
	const ImVec2	size = {390, 400};
	std::string		title;

	std::vector<std::string>	output;
	char						input[256];
};

Panel::uptr_t panel_monitor_create(UIContext *ctx, struct ImVec2 pos) {
	return std::make_unique<PanelMonitor>(ctx, pos);
}
