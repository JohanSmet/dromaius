// gui/panel_control.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to control the emulator

#include "panel_control.h"
#include "gui/imgui_ex.h"
#include "ui_context.h"

#include "context.h"
#include "device.h"
#include "chip_oscillator.h"
#include "signal_line.h"

#include "widgets.h"

#include <algorithm>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

constexpr static const char *MACHINE_NAMES[] = {
	"Commodore PET - 2001N",
	"Commodore PET - 2001N (lite)",
	"Minimal 6502"
};

class PanelControl : public Panel {
public:
	PanelControl(UIContext *ctx, ImVec2 pos, Oscillator *oscillator,
				 StepSignal step_next_instruction, std::initializer_list<StepSignal> step_clocks) :
			Panel(ctx),
			position(pos),
			oscillator(oscillator),
			step_next_instruction(step_next_instruction),
			step_clocks(step_clocks) {
		if (oscillator->frequency >= 1000000) {
			ui_freq_unit_idx = 2;
		} else if (oscillator->frequency >= 1000) {
			ui_freq_unit_idx = 1;
		}
	}

	void init() override {
		button_run_pause_width = std::max(ImGuiEx::ButtonWidth(txt_sim_run), ImGuiEx::ButtonWidth(txt_sim_pause));

		freq_column_0_width = std::max(ImGuiEx::ButtonWidth(txt_freq_header_type), ImGuiEx::ButtonWidth(txt_freq_normal));
		freq_column_0_width = std::max(freq_column_0_width, ImGuiEx::ButtonWidth(txt_freq_actual));
		freq_column_0_width = std::max(freq_column_0_width, ImGuiEx::ButtonWidth(txt_freq_target));

		freq_combo_width = ImGuiEx::ButtonWidth(FREQUENCY_UNITS[0]);
		for (size_t i = 1; i < sizeof(FREQUENCY_UNITS) / sizeof(FREQUENCY_UNITS[0]); ++i) {
			freq_combo_width = std::max(freq_combo_width, ImGuiEx::ButtonWidth(FREQUENCY_UNITS[i]));
		}
		freq_combo_width += ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x * 2.0f;
	}


	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		ImGui::Begin(title);

			ImGui::AlignTextToFramePadding();

			// choose which machine to emulate
			ImGui::Text("Machine");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::Combo("##machine", (int *) &ui_context->config.machine_type, MACHINE_NAMES, sizeof(MACHINE_NAMES) / sizeof(MACHINE_NAMES[0]))) {
				ui_context->switch_machine_requested = true;
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// execution control
			bool sim_paused = dms_is_paused(ui_context->dms_ctx);
			if (sim_paused) {
				if (ImGui::Button(txt_sim_run, {button_run_pause_width, 0.0f})) {
					dms_run(ui_context->dms_ctx);
				}
			} else {
				if (ImGui::Button(txt_sim_pause, {button_run_pause_width, 0.0f})) {
					dms_pause(ui_context->dms_ctx);
				}
			}

			ImGui::SameLine();

			if (ImGui::Button(txt_reset_soft)) {
				Device *device = dms_get_device(ui_context->dms_ctx);
				device->reset(device);
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// debug - stepping
			ImGui::BeginDisabled(!sim_paused);

			if (ImGui::Button(txt_step_single)) {
				dms_single_step(ui_context->dms_ctx);
			}
			ImGui::SameLine();

			if (!step_clocks.empty()) {
				if (ImGui::Button(txt_step_clock)) {
					auto &clk = step_clocks[step_clock_sel];
					dms_step_signal(ui_context->dms_ctx, clk.signal, clk.pos_edge, clk.neg_edge);
				}
				ImGui::SameLine();

				ImGui::SetNextItemWidth(-1);
				auto *signal_pool = ui_context->device->simulator->signal_pool;
				if (ImGui::BeginCombo("##stepClock", signal_get_name(signal_pool, step_clocks[step_clock_sel].signal))) {
					for (size_t i = 0; i < step_clocks.size(); ++i) {
						bool is_selected = (i == step_clock_sel);
						if (ImGui::Selectable(signal_get_name(signal_pool, step_clocks[i].signal), is_selected)) {
							step_clock_sel = i;
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}
			}

			if (!signal_is_undefined(step_next_instruction.signal)  && ImGui::Button(txt_step_instruction, {-1.0f, 0.0f})) {
				dms_step_signal(ui_context->dms_ctx, step_next_instruction.signal, step_next_instruction.pos_edge, step_next_instruction.neg_edge);
			}

			ImGui::EndDisabled();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// simulation frequency
			if (ImGui::BeginTable("table_freq", 2)) {

				// header
				ImGui::TableSetupColumn(txt_freq_header_type, ImGuiTableColumnFlags_WidthFixed, freq_column_0_width);
				ImGui::TableSetupColumn(txt_freq_header_freq);
				ImGui::TableHeadersRow();

				// normal frequency
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text(txt_freq_normal);
				ImGui::TableSetColumnIndex(1);
				ui_text_frequency(oscillator->frequency);

				// actual frequency
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text(txt_freq_actual);
				ImGui::TableSetColumnIndex(1);
				double actual_ratio = dms_simulation_speed_ratio(ui_context->dms_ctx);
				ui_text_frequency((int64_t) ((double) oscillator->frequency * actual_ratio));

				// target frequency
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text(txt_freq_target);
				ImGui::TableSetColumnIndex(1);

				int freq = (int) ((float) oscillator->frequency * speed_ratio);
				int new_freq = freq / FREQUENCY_SCALE[ui_freq_unit_idx];

				ImGui::SetNextItemWidth(-freq_combo_width);
				ImGui::DragInt("##freq", &new_freq, 1, 1, 2000);
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::Combo("##target", &ui_freq_unit_idx, FREQUENCY_UNITS, sizeof(FREQUENCY_UNITS) / sizeof(FREQUENCY_UNITS[0]));

				new_freq *= FREQUENCY_SCALE[ui_freq_unit_idx];
				if (new_freq != freq) {
					speed_ratio = (float) new_freq / (float) oscillator->frequency;
					dms_change_simulation_speed_ratio(ui_context->dms_ctx, speed_ratio);
				}

				ImGui::EndTable();

			}

		ImGui::End();
	}

	void ui_text_frequency(int64_t freq) {
		if (freq > 1000000) {
			ImGui::Text("%3.3f MHz", static_cast<float>(freq) / 1000000.0f);
		} else if (freq > 1000) {
			ImGui::Text("%3.3f KHz", static_cast<float>(freq) / 1000.0f);
		} else {
			ImGui::Text("%" PRId64 " Hz", freq);
		}
	}

private:
	static constexpr const char *txt_machine = "Machine";

	static constexpr const char *txt_sim_run = "Run Simulation";
	static constexpr const char *txt_sim_pause = "Pause Simulation";
	static constexpr const char *txt_reset_soft = "Soft Reset";

	static constexpr const char *txt_step_single = "Single Step";
	static constexpr const char *txt_step_clock  = "Step Clock";
	static constexpr const char *txt_step_instruction  = "Run Until Next Instruction";

	static constexpr const char *txt_freq_header_type = "Type";
	static constexpr const char *txt_freq_header_freq = "Frequency";
	static constexpr const char *txt_freq_normal = "Normal";
	static constexpr const char *txt_freq_actual = "Actual";
	static constexpr const char *txt_freq_target = "Target";

private:
	ImVec2					position;
	const ImVec2			size = {330, 0};
	Oscillator *			oscillator;

	StepSignal				step_next_instruction;
	std::vector<StepSignal> step_clocks;
	size_t					step_clock_sel = 0;

	float					button_run_pause_width;
	float					freq_column_0_width;
	float					freq_combo_width;


	int						ui_freq_unit_idx = 0;
	static constexpr const char *FREQUENCY_UNITS[] = {"Hz", "KHz", "MHz"};
	static constexpr int FREQUENCY_SCALE[] = {1, 1000, 1000000};

	static constexpr const char *title = "Emulator Control";
	float						speed_ratio = 1.0f;
};

Panel::uptr_t panel_control_create(UIContext *ctx, ImVec2 pos, Oscillator *oscillator,
									StepSignal step_next_instruction,
									std::initializer_list<StepSignal> step_clocks) {
	return std::make_unique<PanelControl>(ctx, pos, oscillator, step_next_instruction, step_clocks);
}
