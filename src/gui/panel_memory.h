// gui/panel_memory.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI-panel to visualize a block of memory 

#ifndef DROMAIUS_GUI_PANEL_MEMORY_H
#define DROMAIUS_GUI_PANEL_MEMORY_H

#include "types.h"
#include "panel.h"

Panel::uptr_t panel_memory_create(struct UIContext *ctx, struct ImVec2 pos, const char *title,
								  const uint8_t *data, size_t data_size, size_t data_offset);

#endif // DROMAIUS_GUI_PANEL_MEMORY_H
