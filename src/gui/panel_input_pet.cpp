// gui/panel_input_pet.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_input_pet.h"
#include "ui_context.h"
#include "imgui_ex.h"

#include "input_keypad.h"

class PanelInputPet : public Panel {
public:
	PanelInputPet(UIContext *ctx, ImVec2 pos, InputKeypad *keypad) :
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

			// keyboard
			auto region = ImGui::GetContentRegionAvail();
			region.y -= 30;
			ImGui::BeginChild("raw", region, false, 0);

			// >> regular keys
			auto origin = ImGui::GetCursorScreenPos();
			for (size_t r = 0; r < 10; ++r) {
				for (size_t c = 0; c < 8; ++c) {
					size_t k = (r * keypad->col_count) + c;
					if (key_positions[k].width == 0) {
						continue;
					}

					ImVec2 pos = {origin.x + key_positions[k].left, origin.y + key_positions[k].top};
					ImGui::SetCursorScreenPos(pos);

					ImGui::PushID((int) k);
					ImGuiEx::RectFilled({pos.x + 1, pos.y+1}, {pos.x + key_positions[k].width - 2, pos.y + KEY_SIZE - 2}, colors[keypad->keys[k]]);
					ImGuiEx::Text(labels[k].c_str(), {key_positions[k].width, KEY_SIZE}, ImGuiEx::TAH_CENTER , ImGuiEx::TAV_CENTER);
					if (ImGui::InvisibleButton(labels[k].c_str(), {key_positions[k].width, KEY_SIZE})) {
						input_keypad_key_pressed(keypad, r, c);
					}
					ImGui::PopID();
				}
			}

			// >> shift lock
			ImVec2 pos = {origin.x + 0.2f * KEY_SIZE, origin.y + 2 * KEY_SIZE};
			ImVec2 size = {KEY_SIZE * 1.5f, KEY_SIZE};
			ImGui::SetCursorScreenPos(pos);

			ImGuiEx::RectFilled({pos.x + 1, pos.y + 1}, {pos.x + size.x - 2, pos.y + size.y - 2}, colors[shift_locked]);
			ImGuiEx::Text("Shift\nLock", size, ImGuiEx::TAH_CENTER, ImGuiEx::TAV_CENTER);

			if (ImGui::InvisibleButton("ShiftLock", size)) {
				shift_locked = !shift_locked;
			}

			ImGui::EndChild();

			if (ImGui::SliderInt("Decay", &decay, 1, 512 * 1024)) {
				input_keypad_set_decay(keypad, decay);
			}
		}
		ImGui::End();
	}

	static std::vector<std::string> default_labels() {
		return {"!", "#", "%", "&", "(", "<-", "Home", "CRSR\n=>",		// row 0
				"\"", "$", "'", "\\", ")", "--", "CRSR\nDN", "DEL",		// row 1
				"q", "e", "t", "u", "o", "UP", "7", "9",				// row 2
				"w", "r", "y", "i", "p", "--", "8", "/",				// row 3
				"a", "d", "g", "j", "l", "--", "4", "6",				// row 4
				"s", "f", "h", "k", ":", "--", "5", "*",				// row 5
				"z", "c", "b", "m", ";", "RETURN", "1", "3",			// row 6
				"x", "v", "n", ",", "?", "--", "2", "+",				// row 7
				"SHIFT", "@", "]", "--", ">", "SHIFT", "0", "-",		// row 8
				"RVS", "[", "SPACE", "<", "STOP", "--", ".", "="		// row 9
		};
	}

private:
	struct KeyPos {
		float left;
		float top;
		float width;
	};

	constexpr static float KEY_SIZE = 32;

	constexpr static KeyPos key_positions[80] = {
		// row 0
		{1*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 0 (!)
		{3*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 1 (#)
		{5*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 2 (%)
		{7*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 3 (&)
		{9*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 4 (()
		{11*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 5 (<-)
		{500,			0*KEY_SIZE, KEY_SIZE},			// column 6 (^H)
		{500+2*KEY_SIZE,0*KEY_SIZE, KEY_SIZE},			// column 7 (Cursor Right)

		// row 1
		{2*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 0 (")
		{4*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 1 ($)
		{6*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 2 (')
		{8*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 3 (\)
		{10*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 4 ())
		{0,				0,			0},					// column 5 (not used)
		{500+1*KEY_SIZE,0*KEY_SIZE, KEY_SIZE},			// column 6 (cursor down)
		{500+3*KEY_SIZE,0*KEY_SIZE, KEY_SIZE},			// column 7 (DEL)

		// row 2
		{1.5f*KEY_SIZE,	1*KEY_SIZE, KEY_SIZE},			// column 0 (q)
		{3.5f*KEY_SIZE,	1*KEY_SIZE, KEY_SIZE},			// column 1 (e)
		{5.5f*KEY_SIZE,	1*KEY_SIZE, KEY_SIZE},			// column 2 (t)
		{7.5f*KEY_SIZE,	1*KEY_SIZE, KEY_SIZE},			// column 3 (u)
		{9.5f*KEY_SIZE,	1*KEY_SIZE, KEY_SIZE},			// column 4 (o)
		{11.5f*KEY_SIZE,1*KEY_SIZE, KEY_SIZE},			// column 5 (UP)
		{500+0*KEY_SIZE,1*KEY_SIZE, KEY_SIZE},			// column 6 (7)
		{500+2*KEY_SIZE,1*KEY_SIZE, KEY_SIZE},			// column 7 (9)

		// row 3
		{2.5f*KEY_SIZE,	1*KEY_SIZE, KEY_SIZE},			// column 0 (w)
		{4.5f*KEY_SIZE,	1*KEY_SIZE, KEY_SIZE},			// column 1 (r)
		{6.5f*KEY_SIZE,	1*KEY_SIZE, KEY_SIZE},			// column 2 (y)
		{8.5f*KEY_SIZE,	1*KEY_SIZE, KEY_SIZE},			// column 3 (i)
		{10.5f*KEY_SIZE,1*KEY_SIZE, KEY_SIZE},			// column 4 (p)
		{0,				0,			0},					// column 5 (not used)
		{500+1*KEY_SIZE,1*KEY_SIZE, KEY_SIZE},			// column 6 (8)
		{500+3*KEY_SIZE,1*KEY_SIZE, KEY_SIZE},			// column 7 (/)

		// row 4
		{1.7f*KEY_SIZE,	2*KEY_SIZE, KEY_SIZE},			// column 0 (a)
		{3.7f*KEY_SIZE,	2*KEY_SIZE, KEY_SIZE},			// column 1 (d)
		{5.7f*KEY_SIZE,	2*KEY_SIZE, KEY_SIZE},			// column 2 (g)
		{7.7f*KEY_SIZE,	2*KEY_SIZE, KEY_SIZE},			// column 3 (j)
		{9.7f*KEY_SIZE,	2*KEY_SIZE, KEY_SIZE},			// column 4 (l)
		{0,				0,			0},					// column 5 (not used)
		{500+0*KEY_SIZE,2*KEY_SIZE, KEY_SIZE},			// column 6 (4)
		{500+2*KEY_SIZE,2*KEY_SIZE, KEY_SIZE},			// column 7 (6)

		// row 5
		{2.7f*KEY_SIZE,	2*KEY_SIZE, KEY_SIZE},			// column 0 (s)
		{4.7f*KEY_SIZE,	2*KEY_SIZE, KEY_SIZE},			// column 1 (f)
		{6.7f*KEY_SIZE,	2*KEY_SIZE, KEY_SIZE},			// column 2 (h)
		{8.7f*KEY_SIZE,	2*KEY_SIZE, KEY_SIZE},			// column 3 (k)
		{10.7f*KEY_SIZE,2*KEY_SIZE, KEY_SIZE},			// column 4 (:)
		{0,				0,			0},					// column 5 (not used)
		{500+1*KEY_SIZE,2*KEY_SIZE, KEY_SIZE},			// column 6 (5)
		{500+3*KEY_SIZE,2*KEY_SIZE, KEY_SIZE},			// column 7 (*)

		// row 6
		{2.2f*KEY_SIZE,	3*KEY_SIZE, KEY_SIZE},			// column 0 (z)
		{4.2f*KEY_SIZE,	3*KEY_SIZE, KEY_SIZE},			// column 1 (c)
		{6.2f*KEY_SIZE,	3*KEY_SIZE, KEY_SIZE},			// column 2 (b)
		{8.2f*KEY_SIZE,	3*KEY_SIZE, KEY_SIZE},			// column 3 (m)
		{10.2f*KEY_SIZE,3*KEY_SIZE, KEY_SIZE},			// column 4 (;)
		{12.7f*KEY_SIZE,2*KEY_SIZE, KEY_SIZE*1.5f},		// column 5 (RETURN)
		{500+0*KEY_SIZE,3*KEY_SIZE, KEY_SIZE},			// column 6 (1)
		{500+2*KEY_SIZE,3*KEY_SIZE, KEY_SIZE},			// column 7 (3)

		// row 7
		{3.2f*KEY_SIZE,	3*KEY_SIZE, KEY_SIZE},			// column 0 (x)
		{5.2f*KEY_SIZE,	3*KEY_SIZE, KEY_SIZE},			// column 1 (v)
		{7.2f*KEY_SIZE,	3*KEY_SIZE, KEY_SIZE},			// column 2 (n)
		{9.2f*KEY_SIZE,	3*KEY_SIZE, KEY_SIZE},			// column 3 (,)
		{11.2f*KEY_SIZE,3*KEY_SIZE, KEY_SIZE},			// column 4 (?)
		{0,				0,			0},					// column 5 (not used)
		{500+1*KEY_SIZE,3*KEY_SIZE, KEY_SIZE},			// column 6 (2)
		{500+3*KEY_SIZE,3*KEY_SIZE, KEY_SIZE},			// column 7 (+)

		// row 8
		{0.2f*KEY_SIZE,	3*KEY_SIZE, KEY_SIZE*2.0f},		// column 0 (SHIFT)
		{0.0f*KEY_SIZE,	0*KEY_SIZE, KEY_SIZE},			// column 1 (@)
		{13.0f*KEY_SIZE,0*KEY_SIZE, KEY_SIZE},			// column 2 (])
		{0,				0,			0},					// column 3 (not used)
		{13.5f*KEY_SIZE,1*KEY_SIZE, KEY_SIZE},			// column 4 (>)
		{12.2f*KEY_SIZE,3*KEY_SIZE,	KEY_SIZE*2.0f},		// column 5 (SHIFT)
		{500+0*KEY_SIZE,4*KEY_SIZE, KEY_SIZE},			// column 6 (0)
		{500+2*KEY_SIZE,4*KEY_SIZE, KEY_SIZE},			// column 7 (-)

		// row 9
		{0.0f*KEY_SIZE,	1*KEY_SIZE, KEY_SIZE*1.5f},		// column 0 (RVS)
		{12.0f*KEY_SIZE,0*KEY_SIZE, KEY_SIZE},			// column 1 ([)
		{1.8f*KEY_SIZE,	4*KEY_SIZE, KEY_SIZE*9.8f},		// column 2 (SPACE)
		{12.5f*KEY_SIZE,1*KEY_SIZE, KEY_SIZE},			// column 3 (<)
		{11.7f*KEY_SIZE,2*KEY_SIZE, KEY_SIZE},			// column 4 (STOP)
		{0,				0,			0},					// column 5 (not used)
		{500+1*KEY_SIZE,4*KEY_SIZE, KEY_SIZE},			// column 6 (.)
		{500+3*KEY_SIZE,4*KEY_SIZE, KEY_SIZE},			// column 7 (=)
	};

private:
	ImVec2			position;
	const ImVec2	size = {0, 0};
	InputKeypad *	keypad;
	bool			shift_locked = false;

	std::vector<std::string>	labels = default_labels();
	int							decay = 64 * 1024;

	constexpr static const char *title = "Keypad";
};

Panel::uptr_t panel_input_pet_create(UIContext *ctx, struct ImVec2 pos, struct InputKeypad *keypad) {
	return std::make_unique<PanelInputPet>(ctx, pos, keypad);
}

