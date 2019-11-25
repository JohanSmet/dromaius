// gui/panel_control.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to control the emulator

#ifndef DROMAIUS_GUI_PANEL_CONTROL_H
#define DROMAIUS_GUI_PANEL_CONTROL_H

#include "types.h"

// forward declarations
struct nk_context;
struct nk_vec2;
struct UIContext;

// functions
void panel_control(struct nk_context *nk_ctx, struct UIContext *ui_ctx, struct nk_vec2 pos);

#endif // DROMAIUS_GUI_PANEL_CONTROL_H
