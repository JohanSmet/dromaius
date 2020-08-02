// gui/panel_control.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to control the emulator

#ifndef DROMAIUS_GUI_PANEL_CONTROL_H
#define DROMAIUS_GUI_PANEL_CONTROL_H

#include "panel.h"

// functions
Panel::uptr_t panel_control_create(class UIContext *ctx, struct ImVec2 pos, struct Oscillator *oscillator);

#endif // DROMAIUS_GUI_PANEL_CONTROL_H
