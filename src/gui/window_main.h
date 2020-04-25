// gui/window_main.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_WINDOW_MAIN_H
#define DROMAIUS_GUI_WINDOW_MAIN_H

#include "ui_context.h"

class WindowMain {
public:
	WindowMain() = default;
	WindowMain(WindowMain& other) = delete;

	void on_start();
	void on_render_frame();

private:
	Device *create_minimal_6502();
	Device *create_commodore_pet();

public:
	constexpr static const char* title = "Dromaius";
private:
	UIContext	ui_context;
};

#endif // DROMAIUS_GUI_WINDOW_MAIN_H
