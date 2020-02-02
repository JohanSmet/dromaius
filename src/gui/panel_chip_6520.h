// gui/panel_chip_6520.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a 6520 Peripheral Interace Adapter

#ifndef DROMAIUS_GUI_PANEL_CHIP_6520_H
#define DROMAIUS_GUI_PANEL_CHIP_6520_H

#include "panel.h"

// functions
Panel::uptr_t panel_chip_6520_create(struct UIContext *ctx, struct ImVec2 pos, struct Chip6520 *pia);

#endif // DROMAIUS_GUI_PANEL_CHIP_6520_H
