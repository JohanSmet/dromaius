// gui/panel_memory_raw.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display raw bytes of memory

#ifndef DROMAIUS_GUI_PANEL_MEMORY_RAW_H
#define DROMAIUS_GUI_PANEL_MEMORY_RAW_H

#include "types.h"

// forward declarations
struct nk_context;
struct nk_vec2;

// functions
void panel_memory_raw(struct nk_context *nk_ctx, struct nk_vec2 pos, const char *title, 
		              uint8_t *data, size_t data_size, size_t data_offset);

#endif // DROMAIUS_GUI_PANEL_MEMORY_RAW_H
