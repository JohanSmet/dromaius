// gui/widgets.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// functions for widgets used in multiple panels

#ifndef DROMAIUS_GUI_WIDGETS_H
#define DROMAIUS_GUI_WIDGETS_H

#include "imgui_ex.h"
#include "signal_types.h"
#include "ui_context.h"
#include <stdint.h>
#include <stdbool.h>
#include <initializer_list>
#include <functional>

static ImVec4 UI_COLOR_SIGNAL_HIGH = {0.0f, 0.9f, 0.0f, 1.0f};
static ImVec4 UI_COLOR_SIGNAL_LOW  = {0.0f, 0.6f, 0.0f, 1.0f};
static ImVec4 UI_COLOR_SIGNAL_HIZ  = {0.7f, 0.7f, 0.7f, 1.0f};

static const char *nibble_bit_string[16] = {
	/*[ 0] =*/ "0000", /*[ 1] =*/ "0001", /*[ 2] =*/ "0010", /*[ 3] =*/ "0011",
	/*[ 4] =*/ "0100", /*[ 5] =*/ "0101", /*[ 6] =*/ "0110", /*[ 7] =*/ "0111",
	/*[ 8] =*/ "1000", /*[ 9] =*/ "1001", /*[10] =*/ "1010", /*[11] =*/ "1011",
	/*[12] =*/ "1100", /*[13] =*/ "1101", /*[14] =*/ "1110", /*[15] =*/ "1111"
};

static inline void ui_register_8bit(float left, const char *name, uint8_t value) {
	ImGui::SetCursorPos({left, ImGui::GetCursorPosY()});
	ImGuiEx::Text(128, ImGuiEx::string_format("%s: ", name), ImGuiEx::TAH_RIGHT);
	ImGuiEx::Text(32,  ImGuiEx::string_format("$%.2x", value), ImGuiEx::TAH_LEFT);
	ImGui::NewLine();
}

static inline void ui_register_8bit_binary(float left, const char *name, uint8_t value) {
	ImGui::SetCursorPos({left, ImGui::GetCursorPosY()});
	ImGuiEx::Text(128, ImGuiEx::string_format("%s: ", name), ImGuiEx::TAH_RIGHT);
	ImGuiEx::Text(64,  ImGuiEx::string_format("%%%s%s", nibble_bit_string[value >> 4], nibble_bit_string[value & 0xf]), ImGuiEx::TAH_LEFT);
	ImGui::NewLine();
}

static inline void ui_register_16bit(float left, const char *name, uint16_t value) {
	ImGui::SetCursorPos({left, ImGui::GetCursorPosY()});
	ImGuiEx::Text(128, ImGuiEx::string_format("%s: ", name), ImGuiEx::TAH_RIGHT);
	ImGuiEx::Text(32,  ImGuiEx::string_format("$%.4x", value), ImGuiEx::TAH_LEFT);
	ImGui::NewLine();
}

static inline void ui_key_value(float left, const char *label, const char *value, float value_width) {
	ImGui::SetCursorPos({left, ImGui::GetCursorPosY()});
	ImGuiEx::Text(128, label, ImGuiEx::TAH_RIGHT);
	ImGuiEx::Text(value_width, value, ImGuiEx::TAH_LEFT);
	ImGui::NewLine();
}

static inline void ui_signal(float left, const char *name, bool value, bool asserted) {
	constexpr const char *STR_VALUE[] = {"Low", "High"};
	constexpr const ImVec4 *SIG_COLOR[] = {&UI_COLOR_SIGNAL_LOW, &UI_COLOR_SIGNAL_HIGH, &UI_COLOR_SIGNAL_HIZ};

	ImGui::SetCursorPos({left, ImGui::GetCursorPosY()});

	ImGuiEx::Text(48, ImGuiEx::string_format("%s: ", name), ImGuiEx::TAH_RIGHT);
	ImGuiEx::TextColored(*SIG_COLOR[value == asserted], 32, STR_VALUE[value], ImGuiEx::TAH_LEFT);
	ImGui::NewLine();
}

static inline void ui_signal(SignalValue value) {
	constexpr const char *STR_VALUE[] = {"Low", "High", "High-Z"};
	constexpr const ImVec4 *SIG_COLOR[] = {&UI_COLOR_SIGNAL_LOW, &UI_COLOR_SIGNAL_HIGH, &UI_COLOR_SIGNAL_HIZ};

	int color_idx = (value == SV_HIGH_Z) ? 2 : value;
	ImGui::TextColored(*SIG_COLOR[color_idx], "%s", STR_VALUE[value]);
}

static inline void ui_signal_short(SignalValue value) {
	constexpr const char *STR_VALUE[] = {"L", "H", "-"};
	constexpr const ImVec4 *SIG_COLOR[] = {&UI_COLOR_SIGNAL_LOW, &UI_COLOR_SIGNAL_HIGH, &UI_COLOR_SIGNAL_HIZ};
	int color_idx = (value == SV_HIGH_Z) ? 2 : value;
	ImGui::TextColored(*SIG_COLOR[color_idx], "%s", STR_VALUE[value]);
}

static inline void ui_signal_bits(float left, const char *label, float label_width, SignalValue *values, uint32_t count) {

	constexpr const char *STR_VALUE[] = {"L", "H", "-"};
	constexpr const ImVec4 *SIG_COLOR[] = {&UI_COLOR_SIGNAL_LOW, &UI_COLOR_SIGNAL_HIGH, &UI_COLOR_SIGNAL_HIZ};

	ImGui::SetCursorPos({left, ImGui::GetCursorPosY()});
	ImGuiEx::Text(label_width, ImGuiEx::string_format("%s: ", label), ImGuiEx::TAH_RIGHT);

	for (uint32_t i = 0; i < count; ++i) {
		ImGui::TextColored(*SIG_COLOR[values[i]], "%s", STR_VALUE[values[i]]);
		ImGui::SameLine(0, 0);
	}
	ImGui::NewLine();
}

bool ui_bit_array_table_start(const char *id, int colums, bool header_column, const char *label = nullptr);
void ui_bit_array_table_end();
void ui_bit_array_table_row(const char *label, int columns, SignalValue *values);

////////////////////////////////////////////////////////////////////////////////
//
// Table/TreeNode widget for use in device hardware overview panels
//

class UITreeAction {
public:
	using action_func_t = std::function<void()>;

	UITreeAction(const char *l, action_func_t f) :
		label(l),
		func(f) {
	}

public:
	const char *		label;
	action_func_t		func;
};

class UITreeNode {
public:
	UITreeNode(const char *lbl) : label(lbl) {
	}

	UITreeNode &add_child(const char * lbl) {
		return children.emplace_back(lbl);
	}

	UITreeNode &add_leaf(const char * lbl) {
		return add_child(lbl);
	}

	UITreeNode &add_action(const char * lbl, UITreeAction::action_func_t func) {
		actions.emplace_back(lbl, func);
		return *this;
	}

	void display() const;

public:
	const char *label;

	std::vector<UITreeNode>		children;
	std::vector<UITreeAction>	actions;
};

class UITree {
public:
	// construction
	UITree() : root("root") {
	}

	UITreeNode &add_category(const char *label) {
		return root.add_child(label);
	}

	// display
	void display() const;

private:
	UITreeNode	root;
};

#endif // DROMAIUS_GUI_WIDGETS_H
