// imgui_ex.cpp - Johan Smet - BSD-3-Clause (see LICENSE)
//
// QOL extensions for imgui

#include "imgui_ex.h"
#include "imgui/imgui_internal.h"

namespace {

inline ImVec2 aligned_position(ImVec2 pos, ImVec2 rect, const char *text, ImGuiEx::TextAlignHor align_hor, ImGuiEx::TextAlignVer align_ver) {

    if (align_hor == ImGuiEx::TAH_LEFT && align_ver == ImGuiEx::TAV_TOP) {
        return pos;
    }

    auto size = ImGui::CalcTextSize(text);

    if (align_hor == ImGuiEx::TAH_RIGHT) {
        pos.x += rect.x - size.x;
    } else if (align_hor == ImGuiEx::TAH_CENTER) {
        pos.x += (rect.x - size.x) * 0.5f;
    }

	if (align_ver == ImGuiEx::TAV_BOTTOM) {
		pos.y += rect.y - size.y;
	} else if (align_ver == ImGuiEx::TAV_CENTER) {
		pos.y += (rect.y - size.y) * 0.5f;
	}

    return pos;
}

} // unnamed namespace

namespace ImGuiEx {

float ButtonWidth(const char *text) {
	const ImGuiStyle &style = ImGui::GetStyle();

    auto text_size = ImGui::CalcTextSize(text);
	return text_size.x + style.FramePadding.x * 2.0f;
}

float LabelHeight(const char *text) {
	const ImGuiStyle &style = ImGui::GetStyle();

    auto text_size = ImGui::CalcTextSize(text);
	return text_size.y + style.FramePadding.y * 2.0f;
}

void Text(float width, const char *text, TextAlignHor align_hor) {
	auto org = ImGui::GetCursorScreenPos();
    auto pos = aligned_position(org, {width, 0.0f}, text, align_hor, TAV_TOP);
    ImGui::SetCursorScreenPos(pos);
    ImGui::Text("%s", text);
	ImGui::SetCursorScreenPos(ImVec2(org.x + width, org.y));
}

void TextColored(const ImVec4 &color, float width, const char *text, TextAlignHor align_hor) {
	auto org = ImGui::GetCursorScreenPos();
    auto pos = aligned_position(org, {width, 0.0f}, text, align_hor, TAV_TOP);
    ImGui::SetCursorScreenPos(pos);
    ImGui::TextColored(color, "%s", text);
	ImGui::SetCursorScreenPos(ImVec2(org.x + width, org.y));
}

void Text(const char *text, ImVec2 size, TextAlignHor align_hor, TextAlignVer align_ver) {
	auto org = ImGui::GetCursorScreenPos();
    auto pos = aligned_position(org, size, text, align_hor, align_ver);
    ImGui::SetCursorScreenPos(pos);
    ImGui::Text("%s", text);
	ImGui::SetCursorScreenPos(ImVec2(org.x, org.y));
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
