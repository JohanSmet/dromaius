// gui/panel_control.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to control the emulator

#include "panel_control.h"
#include "ui_context.h"

#include "context.h"
#include "device.h"
#include "chip_oscillator.h"

#include "widgets.h"

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

class PanelControl : public Panel {
public:
	PanelControl(UIContext *ctx, ImVec2 pos, Oscillator *oscillator, StepSignal step_next_instruction) :
		Panel(ctx),
		position(pos),
		oscillator(oscillator),
		step_next_instruction(step_next_instruction) {
		if (oscillator->frequency >= 1000000) {
			ui_freq_unit_idx = 2;
		} else if (oscillator->frequency >= 1000) {
			ui_freq_unit_idx = 1;
		}
	}

	void display() override {
		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		ImGui::Begin(title);

			if (ImGui::Button("Single")) {
				dms_single_step(ui_context->dms_ctx);
			}
			ImGui::SameLine();

			if (step_next_instruction.signal.count > 0 && ImGui::Button("Next Instruction")) {
				dms_step_signal(ui_context->dms_ctx, step_next_instruction.signal, step_next_instruction.pos_edge, step_next_instruction.neg_edge);
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
				Device *device = dms_get_device(ui_context->dms_ctx);
				device->reset(device);
			}

			ImGui::Spacing();
			ImGui::Separator();

			// real speed of simulation
			double actual_ratio = dms_simulation_speed_ratio(ui_context->dms_ctx);
			ui_frequency("Real frequency: ", (int64_t) ((double) oscillator->frequency * actual_ratio));

			// target speed of simulation
			int freq = (int) ((float) oscillator->frequency * speed_ratio);
			int new_freq = freq / FREQUENCY_SCALE[ui_freq_unit_idx];

			ImGui::SetNextItemWidth(160);
			ImGui::DragInt("##freq", &new_freq, 1, 1, 2000);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(48);
			ImGui::Combo("Target", &ui_freq_unit_idx, FREQUENCY_UNITS, sizeof(FREQUENCY_UNITS) / sizeof(FREQUENCY_UNITS[0]));

			new_freq *= FREQUENCY_SCALE[ui_freq_unit_idx];
			if (new_freq != freq) {
				speed_ratio = (float) new_freq / (float) oscillator->frequency;
				dms_change_simulation_speed_ratio(ui_context->dms_ctx, speed_ratio);
			}

		ImGui::End();
	}

	void ui_frequency(const char *label, int64_t freq) {

		if (freq > 1000000) {
			ui_key_value(0, label, ImGuiEx::string_format("%3.3f MHz", static_cast<float>(freq) / 1000000.0f).c_str(), 64);
		} else if (freq > 1000) {
			ui_key_value(0, label, ImGuiEx::string_format("%3.3f KHz", static_cast<float>(freq) / 1000.0f).c_str(), 64);
		} else {
			ui_key_value(0, label, ImGuiEx::string_format("%ld Hz", freq).c_str(), 64);
		}
	}

private:
	ImVec2					position;
	const ImVec2			size = {330, 0};
	Oscillator *			oscillator;

	StepSignal				step_next_instruction;


	int						ui_freq_unit_idx = 0;
	static constexpr const char *FREQUENCY_UNITS[] = {"Hz", "KHz", "MHz"};
	static constexpr int FREQUENCY_SCALE[] = {1, 1000, 1000000};

	static constexpr const char *title = "Emulator Control";
	float						speed_ratio = 1.0f;
};

Panel::uptr_t panel_control_create(UIContext *ctx, ImVec2 pos, Oscillator *oscillator, StepSignal step_next_instruction) {
	return std::make_unique<PanelControl>(ctx, pos, oscillator, step_next_instruction);
}
