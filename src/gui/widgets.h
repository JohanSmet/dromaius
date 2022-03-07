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

bool ui_bit_array_table_start(const char *id, int colums, bool header_column);
void ui_bit_array_table_end();
void ui_bit_array_table_row(const char *label, int columns, SignalValue *values);

////////////////////////////////////////////////////////////////////////////////
//
// Table/TreeNode widget for use in device hardware overview panels
//

template <typename device_t>
struct UITreeAction {
	using action_func_t = std::function<Panel::uptr_t(UIContext *, device_t *)>;

	const char *		label = nullptr;
	int32_t				button_id = 0;
	action_func_t		func = nullptr;
};

template <typename device_t>
struct UITreeNode {

	using callback_func_t = std::function<void(int32_t button_id)>;

	const char *									label;
	std::initializer_list<UITreeAction<device_t>>	actions;
	std::initializer_list<UITreeNode>				children;

	void display_node(UIContext *ui_context, device_t *device, callback_func_t callback = nullptr) const {

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if (children.size() > 0) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			bool open = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_SpanFullWidth);
			if (open) {
				for (auto &child : children) {
					child.display_node(ui_context, device, callback);
				}
				ImGui::TreePop();
			}
		} else {
			ImGui::AlignTextToFramePadding();
			ImGui::TreeNodeEx(label,	ImGuiTreeNodeFlags_Leaf |
										ImGuiTreeNodeFlags_Bullet |
										ImGuiTreeNodeFlags_NoTreePushOnOpen |
										ImGuiTreeNodeFlags_SpanFullWidth);
			ImGui::TableNextColumn();
			ImGui::PushID(this);
			for (auto &action : actions) {
				if (action.func && ImGui::Button(action.label)) {
					auto pnl = action.func(ui_context, device);
					if (pnl) {
						ui_context->panel_add(std::move(pnl));
					}
				}
				if (action.button_id > 0 && callback && ImGui::Button(action.label)) {
					callback(action.button_id);
				}
				ImGui::SameLine();
			}
			ImGui::PopID();
		}
	}
};

template <typename device_t>
struct UITree {
	using data_t = std::initializer_list<UITreeNode<device_t>>;
	using callback_func_t = std::function<void(int32_t button_id)>;

	static void display(data_t hardware, UIContext *ui_context, device_t *device, callback_func_t callback = nullptr) {
		ImGui::PushID(device);
		if (ImGui::BeginTable("table", 2, ImGuiTableFlags_RowBg)) {

			ImGui::TableSetupColumn("Device", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Device", ImGuiTableColumnFlags_WidthFixed);

			for (auto &root: hardware) {
				root.display_node(ui_context, device, callback);
			}

			ImGui::EndTable();
		}
		ImGui::PopID();
	}
};

#define UI_TREE_NODE(l)		{ l, {}, {
#define UI_TREE_NODE_END	} }

#define UI_TREE_LEAF(l)		{ l, {
#define UI_TREE_LEAF_END	}, {}}

#define UI_TREE_ACTION_PANEL(l)	{ l, 0, [] ([[maybe_unused]] auto *ctx, [[maybe_unused]] auto *dev) -> auto {
#define UI_TREE_ACTION_CALLBACK(l, i) {l, i, nullptr }
#define UI_TREE_ACTION_END	}}


#endif // DROMAIUS_GUI_WIDGETS_H
