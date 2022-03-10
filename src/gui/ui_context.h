// gui/ui_context.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_CONTEXT_H
#define DROMAIUS_GUI_CONTEXT_H

#include "types.h"
#include "config.h"
#include "panel.h"

#include <vector>
#include "std_helper.h"

class UIContext {
public:
	UIContext(const Config &config) : config(config) {
	};

	// device management
	void switch_machine(MachineType machine);

	// UI management
	void setup_ui();
	void setup_ui(struct GLFWwindow *window);
	void draw_ui();

	// panels
	void panel_add(Panel::uptr_t panel);
	void panel_close(Panel *panel);
	void panel_close_all();

	std::string unique_panel_id(const char *title) {
		std::string result = title;
		result += "##";
		result += std::to_string(panel_id++);
		return result;
	}

	// helper functions
private:
	void create_device(MachineType machine);
	void create_minimal_6502();
	void create_commodore_pet(bool lite);

	void setup_dockspace();

// member variables
public:
	Config config;
	struct DmsContext *dms_ctx = nullptr;
	struct GLFWwindow *glfw_window = nullptr;

	struct Device * device = nullptr;
	int64_t			last_pc = 0;

	bool			switch_machine_requested = false;

	unsigned int	dock_id_main = 0;
	unsigned int	dock_id_left_top = 0;
	unsigned int	dock_id_left_mid = 0;
	unsigned int	dock_id_left_bot = 0;

private:
	std::vector<Panel::uptr_t>	panels;
	std::vector<Panel::uptr_t>	new_panels;
	int							panel_id = 0;
};

#endif // DROMAIUS_GUI_CONTEXT_H
