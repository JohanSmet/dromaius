// gui/panel_signals.cpp - Johan Smet - BSD-3-Clause (see LICENSE)
//
// visualize signal state

#include "panel_signals.h"
#include "signal_line.h"
#include "context.h"
#include "ui_context.h"
#include "device.h"

#include "imgui_ex.h"
#include <vector>

class PanelSignals : public Panel {
public:
	PanelSignals(UIContext *ctx, ImVec2 pos) :
		Panel(ctx),
		position(pos) {
		input[0] = '\0';
		title = ui_context->unique_panel_id("Signals");
	}

	void display() override {

		auto signal_pool = ui_context->device->simulator->signal_pool;

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {

			ImGui::Columns(4, "signal_colums");
			ImGui::Separator();
			ImGui::Text("Signal"); ImGui::NextColumn();
			ImGui::Text("Current"); ImGui::NextColumn();
			ImGui::Text("Next"); ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::Separator();

			// tracked signals
			std::vector<std::vector<int>::difference_type> to_remove;

			for (size_t i = 0; i < signal_names.size(); ++i) {
				ImGui::PushID(&signals[i]);
				ImGui::Text("%s", signal_names[i].c_str()); ImGui::NextColumn();
				ImGui::Text(signal_read(signal_pool, signals[i]) ? "true" : "false"); ImGui::NextColumn();
				ImGui::Text(signal_read_next(signal_pool, signals[i]) ? "true" : "false"); ImGui::NextColumn();
				if (ImGui::Button("Remove")) {
					to_remove.push_back(static_cast<decltype(to_remove)::difference_type>(i));
				}
				ImGui::NextColumn();
				ImGui::Separator();
				ImGui::PopID();
			}

			for (auto remove_idx : to_remove) {
				signal_names.erase(signal_names.begin() + remove_idx);
				signals.erase(signals.begin() + remove_idx);
			}

			// add new signal
			if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
				ImGui::SetKeyboardFocusHere(0);
			}

			if (ImGui::InputText("##input", input, sizeof(input) - 1, ImGuiInputTextFlags_EnterReturnsTrue)) {
				auto signal = signal_by_name(signal_pool, input);

				if (!signal_is_undefined(signal)) {
					signal_names.push_back(input);
					signals.push_back(signal);

					input[0] = '\0';
					ImGui::SetItemDefaultFocus();
				}
			}
		}
		ImGui::End();
	}

private:
	ImVec2			position;
	const ImVec2	size = {390, 400};
	std::string		title;

	std::vector<std::string>	signal_names;
	std::vector<Signal>			signals;
	char						input[256];
};

Panel::uptr_t panel_signals_create(UIContext *ctx, struct ImVec2 pos) {
	return std::make_unique<PanelSignals>(ctx, pos);
}
