// gui/panel_control.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel that shows information about the clock signal

#ifndef DROMAIUS_GUI_PANEL_CLOCK_H
#define DROMAIUS_GUI_PANEL_CLOCK_H

#include "panel.h"

Panel::uptr_t panel_clock_create(struct UIContext *ctx, struct ImVec2 pos, struct Clock *clock);

#endif // DROMAIUS_GUI_PANEL_CLOCK_H
