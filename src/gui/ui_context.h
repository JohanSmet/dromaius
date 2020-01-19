// gui/ui_context.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_CONTEXT_H
#define DROMAIUS_GUI_CONTEXT_H

#include "types.h"

typedef struct UIContext {
	struct DmsContext *dms_ctx;
	struct PanelMemory *pnl_ram;
	struct PanelMemory *pnl_rom;
	struct PanelControl *pnl_control;
	struct PanelMonitor *pnl_monitor;
	struct PanelChip6520 *pnl_pia;
	struct PanelChipHd44780 *pnl_lcd;
	struct DevMinimal6502 *device;
	uint64_t	last_pc;
} UIContext;

#endif // DROMAIUS_GUI_CONTEXT_H
