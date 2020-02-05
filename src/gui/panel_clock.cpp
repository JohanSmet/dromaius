// gui/panel_clock.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_clock.h"
#include "ui_context.h"

#include "clock.h"
#include "widgets.h"

#define	SIGNAL_POOL			clock->signal_pool
#define SIGNAL_COLLECTION	clock->signals

class PanelClock : public Panel {
public:

	PanelClock(UIContext *ctx, ImVec2 pos, Clock *clock) :
		Panel(ctx),
		position(pos),
		clock(clock) {
	}

	void display() override {
		constexpr static const char *units[] = {"Hz", "KHz", "MHz"};
		static const int factor[] = {1, 1000, 1000000};

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title)) {
			// configured frequency
			int unit_idx = 0;
			int value = clock->conf_frequency;

			if (value >= 1000000) {
				value /= 1000000;
				unit_idx = 2;
			} else if (value >= 1000) {
				value /= 1000;
				unit_idx = 1;
			}


			ImGui::SetNextItemWidth(160);
			ImGui::DragInt("##target", &value, 1, 1, 999);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(48);
			ImGui::Combo("Target frequency", &unit_idx, units, sizeof(units) / sizeof(units[0]));

			if (clock->conf_frequency != value * factor[unit_idx]) {
				clock_set_frequency(clock, value * factor[unit_idx]);
			}

			// real frequency
			ui_frequency("Real frequency: ", clock->real_frequency);

			// state
			ui_key_value(0, "Output: ", SIGNAL_NEXT_BOOL(clock) ? "High" : "Low", 64);

			// cycle count
			ui_key_value(0, "Cycle: ", ImGuiEx::string_format("%ld", clock->cycle_count).c_str(), 64);
		}

		ImGui::End();

	}

	void ui_frequency(const char *label, int64_t freq) {

		if (freq > 1000000) {
			ui_key_value(0, label, ImGuiEx::string_format("%3.f MHz", static_cast<float>(freq) / 1000000.0f).c_str(), 64);
		} else if (freq > 1000) {
			ui_key_value(0, label, ImGuiEx::string_format("%3.f KHz", static_cast<float>(freq) / 1000.0f).c_str(), 64);
		} else {
			ui_key_value(0, label, ImGuiEx::string_format("%ld Hz", freq).c_str(), 64);
		}
	}

private:
	ImVec2			position;
	const ImVec2	size = {360, 0};
	Clock *			clock;

	constexpr static const char *title = "Clock";
};

Panel::uptr_t panel_clock_create(UIContext *ctx, struct ImVec2 pos, struct Clock *clock) {
	return std::make_unique<PanelClock>(ctx, pos, clock);
}
