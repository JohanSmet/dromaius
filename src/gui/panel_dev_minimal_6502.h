// gui/panel_dev_minimal_6502.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_PANEL_DEV_MINIMAL_6502_H
#define DROMAIUS_GUI_PANEL_DEV_MINIMAL_6502_H

#include "panel.h"

Panel::uptr_t panel_dev_minimal_6502_create(class UIContext *ctx, struct ImVec2 pos, struct DevMinimal6502 *device);

#endif // DROMAIUS_GUI_PANEL_DEV_MINIMAL_6502_H
