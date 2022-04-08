// gui/panel_logic_analyzer.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// visualize signal state

#ifndef DROMAIUS_GUI_PANEL_LOGIC_ANALYZER_H
#define DROMAIUS_GUI_PANEL_LOGIC_ANALYZER_H

#include "panel.h"

Panel::uptr_t panel_logic_analyzer_create(class UIContext *ctx, struct ImVec2 pos);

#endif // DROMAIUS_GUI_PANEL_LOGIC_ANALYZER_H
