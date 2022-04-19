// gui/panel_logic_analyzer.cpp - Johan Smet - BSD-3-Clause (see LICENSE)
//
// visualize signal state

#include "panel_logic_analyzer.h"
#include "signal_history.h"
#include "context.h"
#include "ui_context.h"
#include "device.h"

#include "imgui_ex.h"
#include <imgui.h>
#include <vector>

class PanelLogicAnalyzer : public Panel {
public:
	PanelLogicAnalyzer(UIContext *ctx, ImVec2 pos) :
		Panel(ctx),
		position(pos) {
		input[0] = '\0';
		title = ui_context->unique_panel_id("LogicAnalyzer");
	}

	void init() override {
		time_scale = NS_TO_PS(1000) / (20.0f * (float) ui_context->device->simulator->tick_duration_ps);	// 20 pixels = 1 µs = 1000 ns == 1 MHz
		enable_history = ui_context->device->simulator->signal_history->capture_active;
	}

	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {

			auto sim = ui_context->device->simulator;

			// header

			// >> enable history keeping
			if (ImGui::Checkbox("Enable", &enable_history)) {
				if (enable_history) {
					signal_history_process_start(sim->signal_history);
				} else {
					signal_history_process_stop(sim->signal_history);
					enable_gtkwave = false;
				}
			}

			ImGui::SameLine();

			// >> enable GtkWave output
#ifdef DMS_GTKWAVE_EXPORT
			if (ImGui::Checkbox("GtkWave export", &enable_gtkwave)) {
				if (enable_gtkwave) {
					signal_history_gtkwave_enable(	sim->signal_history, "dromaius.lxt",
													arrlenu(diagram_data.signals),
													diagram_data.signals,
													sim->signal_pool->signals_name);
				} else {
					signal_history_gtkwave_disable(sim->signal_history);
				}
			}
#endif // DMS_GTKWAVE_EXPORT

			ImGui::Combo("##profiles", &current_profile,
						 signal_history_profile_names(sim->signal_history),
						 (int) signal_history_profile_count(sim->signal_history));

			if (ImGui::Button(" Add ##add_profile")) {
				add_profile_signals();
			}
			// >> add new signal
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

			// divider between header and body
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// body
			auto region = ImGui::GetContentRegionAvail();
			ImGui::BeginChild("display", region, false, 0);

			// >> calculate what time interval to fetch data for
			int64_t available_time = (int64_t) ((region.x - (BORDER_WIDTH * 2.0f)) * time_scale);
			diagram_data.time_end = sim->current_tick;
			diagram_data.time_begin = diagram_data.time_end - available_time;
			if (diagram_data.time_begin < 0) {
				diagram_data.time_begin = 0;
				diagram_data.time_end = available_time;
			}

			// >> fetch data
			signal_history_diagram_data(ui_context->device->simulator->signal_history, &diagram_data);

			// >> display data
			for (size_t si = 0; si < arrlenu(diagram_data.signals); ++si) {
				handle_signal(si);
			}

			ImGui::EndChild();

		}
		ImGui::End();
	}

private:
	static constexpr float BORDER_WIDTH = 10;
	static constexpr  auto COLOR_SIGNAL = IM_COL32(0, 175, 0, 255);
	static constexpr  auto COLOR_TEXT = IM_COL32(80, 30, 80, 255);
	static constexpr  auto COLOR_LABEL = IM_COL32(150, 150, 150, 200);
	static constexpr float TRUE_OFFSET = -20.0f;

	void handle_signal(size_t signal_idx) {

		auto draw_list = ImGui::GetWindowDrawList();
		auto origin = ImGui::GetCursorScreenPos();
		auto sim = ui_context->device->simulator;

		float signal_y = origin.y + ((float) signal_idx * 40.0f) + 30.0f;

		auto diagram_x = [&](int64_t t) -> float {
			return origin.x + BORDER_WIDTH + (float) (t - diagram_data.time_begin) / time_scale;
		};

		// draw from right to left (starting at current position)
		if (diagram_data.signal_start_offsets[signal_idx] != diagram_data.signal_start_offsets[signal_idx + 1]) {
			size_t sample = diagram_data.signal_start_offsets[signal_idx];
			draw_list->PathClear();
			draw_list->PathLineTo({diagram_x(sim->current_tick), signal_y + (diagram_data.samples_value[sample] * TRUE_OFFSET)});

			for (; sample < diagram_data.signal_start_offsets[signal_idx+1]; ++sample) {
				float signal_x = diagram_x(diagram_data.samples_time[sample]);
				draw_list->PathLineTo({signal_x, signal_y + (diagram_data.samples_value[sample] * TRUE_OFFSET)});
				draw_list->PathLineTo({signal_x, signal_y + (!diagram_data.samples_value[sample] * TRUE_OFFSET)});
			}

			draw_list->PathStroke(COLOR_SIGNAL, 0, 2.0f);
		}

		// signal-name
		const char *label = signal_names[signal_idx].c_str();
		const ImVec2 label_pos = {origin.x + BORDER_WIDTH, signal_y - 20};
		const float label_padding = 2.0f;
		auto text_size = ImGui::CalcTextSize(label);

		draw_list->AddRectFilled({label_pos.x - label_padding, label_pos.y - label_padding},
								 {label_pos.x + text_size.x + label_padding, label_pos.y + text_size.y + label_padding},
								 COLOR_LABEL,
								 4.0f);
		draw_list->AddText(label_pos, COLOR_TEXT, label);
	}

private:
	bool valid_input_signal() {
		if (enable_gtkwave) {
			return false;
		}
		auto signal = signal_by_name(ui_context->device->simulator->signal_pool, input);
		return signal_is_undefined(signal);
	}

	void add_input_signal() {
		auto signal = signal_by_name(ui_context->device->simulator->signal_pool, input);

		if (!signal_is_undefined(signal)) {
			signal_names.push_back(input);
			arrpush(diagram_data.signals, signal);

			input[0] = '\0';
			ImGui::SetItemDefaultFocus();
		}
	}

	void add_profile_signals() {
		Simulator *sim = ui_context->device->simulator;
		Signal *signals = signal_history_profile_signals(sim->signal_history, (uint32_t) current_profile);
		const char **aliases = signal_history_profile_signal_aliases(sim->signal_history, (uint32_t) current_profile);

		for (size_t i = 0; i < arrlenu(signals); ++i) {
			auto sig_name = signal_get_name(sim->signal_pool, signals[i]);
			if (aliases[i] && strcmp(sig_name, aliases[i])) {
				std::string full_name = aliases[i];
				full_name += " (";
				full_name += sig_name;
				full_name += ")";
				signal_names.push_back(full_name);
			} else {
				signal_names.push_back(sig_name);
			}

			arrpush(diagram_data.signals, signals[i]);
		}
	}

private:
	ImVec2			position;
	const ImVec2	size = {390, 400};
	std::string		title;

	bool						enable_history = false;
	bool						enable_gtkwave = false;
	int							current_profile = 0;
	float						time_scale = 0;

	std::vector<std::string>	signal_names;
	SignalHistoryDiagramData	diagram_data = {};
	char						input[256];
};

Panel::uptr_t panel_logic_analyzer_create(UIContext *ctx, struct ImVec2 pos) {
	return std::make_unique<PanelLogicAnalyzer>(ctx, pos);
}
