// gui/panel_memory_disasm.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display a disassembly of a memory block

#ifndef DROMAIUS_GUI_PANEL_MEMORY_DISASM_H
#define DROMAIUS_GUI_PANEL_MEMORY_DISASM_H

#include "types.h"

// forward declarations
struct nk_context;
struct nk_vec2;
struct UIContext;

// functions
void panel_memory_disasm(struct nk_context *nk_ctx, struct UIContext *ui_ctx, struct nk_vec2 pos, const char *title, 
						 uint8_t *data, size_t data_size, size_t data_offset, uint64_t pc);

#endif // DROMAIUS_GUI_PANEL_MEMORY_DISASM_H
