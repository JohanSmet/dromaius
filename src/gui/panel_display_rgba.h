// gui/panel_display_rgba.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_PANEL_DISPLAY_RGBA_H
#define DROMAIUS_GUI_PANEL_DISPLAY_RGBA_H

#include "panel.h"

Panel::uptr_t panel_display_rgba_create(class UIContext *ctx, struct ImVec2 pos, struct DisplayRGBA *display);

#endif // DROMAIUS_GUI_PANEL_DISPLAY_RGBA_H
