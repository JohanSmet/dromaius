// gui/panel_chip_6520.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a 6520 Peripheral Interace Adapter

#ifndef DROMAIUS_GUI_PANEL_CHIP_6520_H
#define DROMAIUS_GUI_PANEL_CHIP_6520_H

#include "types.h"

// forward declarations
struct nk_context;
struct nk_vec2;
struct DmsContext;

struct PanelChip6520;
struct Chip6520;

// functions
struct PanelChip6520 *panel_chip_6520_init(
						struct nk_context *nk_ctx,
						struct nk_vec2 pos,
						struct DmsContext *dms_ctx,
						struct Chip6520 *pia);
void panel_chip_6520_display(struct PanelChip6520 *pnl);
void panel_chip_6520_release(struct PanelChip6520 *pnl);

#endif // DROMAIUS_GUI_PANEL_CHIP_6520_H
