// imgui_ex.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// QOL extensions for imgui

#ifndef DROMAIUS_GUI_IMGUI_EX_H
#define DROMAIUS_GUI_IMGUI_EX_H

#include "imgui/imgui.h"
#include <string>
#include <cassert>

namespace ImGuiEx {

enum TextAlignHor {
    TAH_LEFT,
    TAH_RIGHT,
    TAH_CENTER
};

enum TextAlignVer {
    TAV_TOP,
    TAV_BOTTOM,
    TAV_CENTER
};

template< typename... Args >
std::string string_format(const char* format, Args... args ) {
  int length = std::snprintf(nullptr, 0, format, args...);
  assert(length >= 0);

  std::string result(length, ' ');
  std::snprintf(result.data(), length + 1, format, args...);

  return result;
}

void Text(float width, const char *text, TextAlignHor align_hor = TAH_LEFT);
void TextColored(const ImVec4 &color, float width, const char *text, TextAlignHor align_hor = TAH_LEFT);

inline void Text(float width, const std::string &text, TextAlignHor align_hor = TAH_LEFT) {
    Text(width, text.c_str(), align_hor);
}

inline void TextColored(const ImVec4 &color, float width, const std::string &text, TextAlignHor align_hor = TAH_LEFT) {
	return TextColored(color, width, text.c_str(), align_hor);
}

void RectFilled(ImVec2 p1, ImVec2 p2, ImU32 col);

} // namespace ImGuiEx

#endif // DROMAIUS_GUI_IMGUI_EX_H
