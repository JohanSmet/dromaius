// gui/panel_ieee488_tester.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_PANEL_IEEE488_TESTER_H
#define DROMAIUS_GUI_PANEL_IEEE488_TESTER_H

#include "panel.h"

// functions
Panel::uptr_t panel_ieee488_tester_create(	class UIContext *ctx, struct ImVec2 pos,
											struct Perif488Tester *ieee_tester
);

#endif // DROMAIUS_GUI_PANEL_IEEE488_TESTER_H
