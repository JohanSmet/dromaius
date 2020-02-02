// gui/ui_context.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_CONTEXT_H
#define DROMAIUS_GUI_CONTEXT_H

#include "types.h"
#include "panel.h"

#include <vector>

struct UIContext {
	struct DmsContext *dms_ctx;

	std::vector<Panel::uptr_t>	panels;

	struct DevMinimal6502 *device;
	uint64_t	last_pc;
};

#endif // DROMAIUS_GUI_CONTEXT_H
