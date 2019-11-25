// gui/panel_cpu_6502.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a MOS 6502

#ifndef DROMAIUS_GUI_PANEL_CPU_6502_H
#define DROMAIUS_GUI_PANEL_CPU_6502_H

// forward declarations
struct nk_context;
struct nk_vec2;
struct Cpu6502;

// functions
void panel_cpu_6502(struct nk_context *nk_ctx, struct nk_vec2 pos, struct Cpu6502 *cpu);

#endif // DROMAIUS_GUI_PANEL_CPU_6502_H
