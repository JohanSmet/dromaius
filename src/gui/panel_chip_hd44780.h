// gui/panel_chip_hd44780.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display the LCD output of a Hitachi HD44780 LCD Controller/Driver

#ifndef DROMAIUS_GUI_PANEL_CHIP_HD44780_H
#define DROMAIUS_GUI_PANEL_CHIP_HD44780_H

#include "types.h"

// forward declarations
struct nk_context;
struct nk_vec2;
struct DmsContext;

struct PanelChipHd44780;
struct ChipHd44780;

// functions
struct PanelChipHd44780 *panel_chip_hd44780_init(
						struct nk_context *nk_ctx,
						struct nk_vec2 pos,
						struct DmsContext *dms_ctx,
						struct ChipHd44780 *lcd);
void panel_chip_hd44780_display(struct PanelChipHd44780 *pnl);
void panel_chip_hd44780_release(struct PanelChipHd44780 *pnl);

#endif // DROMAIUS_GUI_PANEL_CHIP_HD44780_H
