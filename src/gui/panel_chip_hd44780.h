// gui/panel_chip_hd44780.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display the LCD output of a Hitachi HD44780 LCD Controller/Driver

#ifndef DROMAIUS_GUI_PANEL_CHIP_HD44780_H
#define DROMAIUS_GUI_PANEL_CHIP_HD44780_H

#include "panel.h"

Panel::uptr_t panel_chip_hd44780_create(class UIContext *ctx, struct ImVec2 pos, struct ChipHd44780 *lcd);

#endif // DROMAIUS_GUI_PANEL_CHIP_HD44780_H
