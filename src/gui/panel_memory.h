// gui/panel_memory.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI-panel to visualize a block of memory

#ifndef DROMAIUS_GUI_PANEL_MEMORY_H
#define DROMAIUS_GUI_PANEL_MEMORY_H

#include "panel.h"

Panel::uptr_t panel_memory_create(class UIContext *ctx, struct ImVec2 pos, const char *title,
								  size_t data_offset, size_t data_size);
void panel_memory_load_fonts();

#endif // DROMAIUS_GUI_PANEL_MEMORY_H
