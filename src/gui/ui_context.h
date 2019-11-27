// gui/ui_context.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_CONTEXT_H
#define DROMAIUS_GUI_CONTEXT_H

#include "types.h"

typedef struct UIContext {
	struct DmsContext *dms_ctx;
	struct DevMinimal6502 *device;
	uint64_t	last_pc;
} UIContext;

#endif // DROMAIUS_GUI_CONTEXT_H
