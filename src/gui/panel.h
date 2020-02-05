// gui/panel_base.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_PANEL_BASE_H
#define DROMAIUS_GUI_PANEL_BASE_H

#include "types.h"

#include <memory>

class Panel {
public:
	using uptr_t = std::unique_ptr<Panel>;

public:
	Panel(class UIContext *ui_context) : ui_context(ui_context) {
	}

public:
	virtual ~Panel() = default;
	virtual void display() = 0;

protected:
	class UIContext *ui_context = nullptr;
};

#endif // DROMAIUS_GUI_PANEL_BASE_H
