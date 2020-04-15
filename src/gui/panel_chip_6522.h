// gui/panel_chip_6522.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a 6522 Versatile Interace Adapter

#ifndef DROMAIUS_GUI_PANEL_CHIP_6522_H
#define DROMAIUS_GUI_PANEL_CHIP_6522_H

#include "panel.h"

// functions
Panel::uptr_t panel_chip_6522_create(class UIContext *ctx, struct ImVec2 pos, struct Chip6522 *via);

#endif // DROMAIUS_GUI_PANEL_CHIP_6522_H
