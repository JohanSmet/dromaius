// gui/panel_signals.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// visualize signal state

#ifndef DROMAIUS_GUI_PANEL_SIGNALS_H
#define DROMAIUS_GUI_PANEL_SIGNALS_H

#include "panel.h"

Panel::uptr_t panel_signals_create(class UIContext *ctx, struct ImVec2 pos);

#endif // DROMAIUS_GUI_PANEL_SIGNALS_H
