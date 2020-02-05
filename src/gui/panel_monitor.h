// gui/panel_monitor.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// simple hardware monitor

#ifndef DROMAIUS_GUI_PANEL_MONITOR_H
#define DROMAIUS_GUI_PANEL_MONITOR_H

#include "panel.h"

Panel::uptr_t panel_monitor_create(class UIContext *ctx, struct ImVec2 pos);

#endif // DROMAIUS_GUI_PANEL_MONITOR_H
