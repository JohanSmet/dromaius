// gui/panel_input_keypad.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_input_keypad.h"
#include "ui_context.h"
#include "imgui_ex.h"

#include "input_keypad.h"

class PanelInputKeypad : public Panel {
public:
	PanelInputKeypad(UIContext *ctx, ImVec2 pos, InputKeypad *keypad) :
		Panel(ctx),
		position(pos),
		keypad(keypad) {
		input_keypad_set_decay(keypad, decay);
	}

	void display() override {
		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		static ImU32 colors[2] = {ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Button)),
								  ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive))};

		if (ImGui::Begin(title)) {
			for (size_t r = 0; r < 4; ++r) {
				for (size_t c = 0; c < 4; ++c) {
					size_t k = (r * keypad->col_count) + c;

					auto pos = ImGui::GetCursorScreenPos();
					ImGuiEx::RectFilled(pos, {pos.x + 32, pos.y + 32}, colors[keypad->keys[k]]);
					ImGuiEx::Text(labels[r*4+c].c_str(), {32,32}, ImGuiEx::TAH_CENTER , ImGuiEx::TAV_CENTER);
					ImGui::InvisibleButton(labels[r*4+c].c_str(), {32,32});

					if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)) {
						input_keypad_key_pressed(keypad, r, c);
					}

					ImGui::SameLine();
				}
				ImGui::NewLine();
			}

			if (ImGui::SliderInt("Decay", &decay, 1, 512 * 1024)) {
				input_keypad_set_decay(keypad, decay);
			}
		}
		ImGui::End();
	}

	static std::vector<std::string> default_labels() {
		return {"1", "2", "3", "A",
				"4", "5", "6", "B",
				"7", "8", "9", "C",
				"*", "0", "#", "D"};
	}

private:
	ImVec2			position;
	const ImVec2	size = {0, 0};
	InputKeypad *	keypad;

	std::vector<std::string>	labels = default_labels();
	int							decay = 64 * 1024;

	constexpr static const char *title = "Keypad";
};

Panel::uptr_t panel_input_keypad_create(UIContext *ctx, struct ImVec2 pos, struct InputKeypad *keypad) {
	return std::make_unique<PanelInputKeypad>(ctx, pos, keypad);
}

