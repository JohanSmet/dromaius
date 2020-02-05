// gui/panel_cpu_6502.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a MOS 6502

#ifndef DROMAIUS_GUI_PANEL_CPU_6502_H
#define DROMAIUS_GUI_PANEL_CPU_6502_H

#include "panel.h"

// functions
Panel::uptr_t panel_cpu_6502_create(class UIContext *ctx, struct ImVec2 pos, struct Cpu6502 *cpu);

#endif // DROMAIUS_GUI_PANEL_CPU_6502_H
