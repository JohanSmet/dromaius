// gui/panel_monitor.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// simple hardware monitor

#ifndef DROMAIUS_GUI_PANEL_MONITOR_H
#define DROMAIUS_GUI_PANEL_MONITOR_H

#include "types.h"

// forward declarations
struct nk_context;
struct nk_vec2;
struct DmsContext;

struct PanelMonitor;

// functions
struct PanelMonitor *panel_monitor_init( 
						struct nk_context *nk_ctx, 
						struct nk_vec2 pos, 
						struct DmsContext *dms_ctx);
void panel_monitor_display(struct PanelMonitor *pnl);
void panel_monitor_release(struct PanelMonitor *pnl);

#endif // DROMAIUS_GUI_PANEL_MONITOR_H
