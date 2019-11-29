// gui/panel_control.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to control the emulator

#ifndef DROMAIUS_GUI_PANEL_CONTROL_H
#define DROMAIUS_GUI_PANEL_CONTROL_H

#include "types.h"

// forward declarations
struct nk_context;
struct nk_vec2;
struct DmsContext;

// functions
struct PanelControl *panel_control_init(struct nk_context *nk_ctx, struct nk_vec2 pos, struct DmsContext *dms_ctx, const char *data_path);
void panel_control_release(struct PanelControl *pnl);
void panel_control_display(struct PanelControl *pnl);

void panel_control_select_rom(struct PanelControl *pnl, int index);
void panel_control_select_ram(struct PanelControl *pnl, int index);

#endif // DROMAIUS_GUI_PANEL_CONTROL_H
