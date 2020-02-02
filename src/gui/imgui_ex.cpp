// imgui_ex.cpp - Johan Smet - BSD-3-Clause (see LICENSE)
//
// QOL extensions for imgui

#include "imgui_ex.h"
#include "imgui/imgui_internal.h"

namespace {

inline ImVec2 aligned_position(ImVec2 pos, float width, const char *text, ImGuiEx::TextAlignHor align_hor) {

    if (align_hor == ImGuiEx::TAH_LEFT) {
        return pos;
    }

    auto size = ImGui::CalcTextSize(text);

    if (align_hor == ImGuiEx::TAH_RIGHT) {
        pos.x += width - size.x;
    } else if (align_hor == ImGuiEx::TAH_CENTER) {
        pos.x += width - (size.x * 0.5f);
    }

    return pos;
}

} // unnamed namespace

namespace ImGuiEx {

void Text(float width, const char *text, TextAlignHor align_hor) {
	auto org = ImGui::GetCursorScreenPos();
    auto pos = aligned_position(org, width, text, align_hor);
    ImGui::SetCursorScreenPos(pos);
    ImGui::Text("%s", text);
	ImGui::SetCursorScreenPos(ImVec2(org.x + width, org.y));
}

void TextColored(const ImVec4 &color, float width, const char *text, TextAlignHor align_hor) {
	auto org = ImGui::GetCursorScreenPos();
    auto pos = aligned_position(org, width, text, align_hor);
    ImGui::SetCursorScreenPos(pos);
    ImGui::TextColored(color, "%s", text);
	ImGui::SetCursorScreenPos(ImVec2(org.x + width, org.y));
}

void RectFilled(ImVec2 p1, ImVec2 p2, ImU32 col) {
    auto min = p1;
    auto max = p2;

	if (max.x < min.x) {
        std::swap(min.x, max.x);
	}

	if (max.y < min.y) {
        std::swap(min.y, max.y);
	}

    ImGui::GetWindowDrawList()->AddRectFilled(min, max, col);
}

} // namespace ImGuiEx
