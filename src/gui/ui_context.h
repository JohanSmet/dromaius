// gui/ui_context.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_CONTEXT_H
#define DROMAIUS_GUI_CONTEXT_H

#include "types.h"
#include "panel.h"

#include <vector>
#include "std_helper.h"

class UIContext {
public:
	void panel_add(Panel::uptr_t panel) {
		panels.push_back(std::move(panel));
	}

	void panel_close(Panel *panel) {
		dms::remove_owner(panels, panel);
	}

	template <typename func_t>
	void panel_foreach(func_t f) {
		for (auto &p : panels) {
			f(p.get());
		}
	}

// member variables
public:
	struct DmsContext *dms_ctx;

	struct DevMinimal6502 *device;
	uint64_t	last_pc;

private:
	std::vector<Panel::uptr_t>	panels;
};

#endif // DROMAIUS_GUI_CONTEXT_H
