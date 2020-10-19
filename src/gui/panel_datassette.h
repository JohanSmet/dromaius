// gui/panel_datassette.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_PANEL_DATASSETTE_H
#define DROMAIUS_GUI_PANEL_DATASSETTE_H

#include "panel.h"

// functions
Panel::uptr_t panel_datassette_create(	class UIContext *ctx, struct ImVec2 pos,
										struct PerifDatassette *datassette
);

#endif // DROMAIUS_GUI_PANEL_DATASSETTE_H
