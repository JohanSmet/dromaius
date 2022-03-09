// gui/panel_signals.cpp - Johan Smet - BSD-3-Clause (see LICENSE)
//
// visualize signal state

#include "panel_signals.h"
#include "signal_line.h"
#include "context.h"
#include "ui_context.h"
#include "device.h"

#include "imgui_ex.h"
#include <imgui.h>
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

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {

			auto signal_pool = ui_context->device->simulator->signal_pool;

			// add new signal
			if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
				ImGui::SetKeyboardFocusHere(0);
			}

			if (ImGui::InputText("##input", input, sizeof(input) - 1, ImGuiInputTextFlags_EnterReturnsTrue)) {
				add_input_signal();
			}

			ImGui::SameLine();

			ImGui::BeginDisabled(valid_input_signal());
			if (ImGui::Button(" Add ##add")) {
				add_input_signal();
			}
			ImGui::EndDisabled();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (!signals.empty() && ImGui::BeginTable("signals", 4, ImGuiTableFlags_Borders)) {

				ImGui::TableSetupColumn("Signal");
				ImGui::TableSetupColumn("Value");
				ImGui::TableSetupColumn("Breakpoint");
				ImGui::TableSetupColumn("##remove");
				ImGui::TableHeadersRow();

				// tracked signals
				std::vector<std::vector<int>::difference_type> to_remove;

				for (size_t i = 0; i < signal_names.size(); ++i) {
					ImGui::PushID(&signals[i]);
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("%s", signal_names[i].c_str());

					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text(signal_read(signal_pool, signals[i]) ? "true" : "false");

					ImGui::TableNextColumn();
					bool sb = dms_breakpoint_signal_is_set(ui_context->dms_ctx, signals[i]);
					if (ImGui::Checkbox("##bp", &sb)) {
						dms_toggle_signal_breakpoint(ui_context->dms_ctx, signals[i]);
					}

					ImGui::TableNextColumn();
					if (ImGui::Button("Remove")) {
						to_remove.push_back(static_cast<decltype(to_remove)::difference_type>(i));
					}

					ImGui::PopID();
				}

				ImGui::EndTable();

				// remove signals
				for (auto remove_idx : to_remove) {
					signal_names.erase(signal_names.begin() + remove_idx);
					signals.erase(signals.begin() + remove_idx);
				}
			}
		}
		ImGui::End();
	}

private:
	bool valid_input_signal() {
		auto signal = signal_by_name(ui_context->device->simulator->signal_pool, input);
		return signal_is_undefined(signal);
	}

	void add_input_signal() {
		auto signal = signal_by_name(ui_context->device->simulator->signal_pool, input);

		if (!signal_is_undefined(signal)) {
			signal_names.push_back(input);
			signals.push_back(signal);

			input[0] = '\0';
			ImGui::SetItemDefaultFocus();
		}
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
