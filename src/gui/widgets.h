// gui/widgets.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// functions for widgets used in multiple panels

#ifndef DROMAIUS_GUI_WIDGETS_H
#define DROMAIUS_GUI_WIDGETS_H

#include "imgui_ex.h"
#include <stdint.h>
#include <stdbool.h>

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
	static const ImVec4 SIG_COLOR[] = { ImVec4(0.8f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.8f, 0.0f, 1.0f)};

	ImGui::SetCursorPos({left, ImGui::GetCursorPosY()});

	ImGuiEx::Text(48, ImGuiEx::string_format("%s: ", name), ImGuiEx::TAH_RIGHT);
	ImGuiEx::TextColored(SIG_COLOR[value == asserted], 32, STR_VALUE[value], ImGuiEx::TAH_LEFT);
	ImGui::NewLine();
}

#endif // DROMAIUS_GUI_WIDGETS_H
