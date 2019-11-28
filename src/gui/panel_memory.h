// gui/panel_memory.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI-panel to visualize a block of memory 

#ifndef DROMAIUS_GUI_PANEL_MEMORY_H
#define DROMAIUS_GUI_PANEL_MEMORY_H

#include "types.h"

// forward declarations
struct nk_context;
struct nk_vec2;

struct PanelMemory;

// functions
struct PanelMemory *panel_memory_init( 
						struct nk_context *nk_ctx, 
						struct nk_vec2 pos, const char *title, 
						const uint8_t *data, size_t data_size, size_t data_offset);
void panel_memory_display(struct PanelMemory *pnl, uint64_t current_pc);
void panel_memory_release(struct PanelMemory *pnl);

#endif // DROMAIUS_GUI_PANEL_MEMORY_H
