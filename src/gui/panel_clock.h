// gui/panel_control.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel that shows information about the clock signal

#ifndef DROMAIUS_GUI_PANEL_CLOCK_H
#define DROMAIUS_GUI_PANEL_CLOCK_H

#include "types.h"

// forward declarations
struct nk_context;
struct nk_vec2;
struct Clock;

// functions
void panel_clock(struct nk_context *nk_ctx, struct Clock *clock, struct nk_vec2 pos);

#endif // DROMAIUS_GUI_PANEL_CLOCK_H
