// gui/panel_control.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to control the emulator

#ifndef DROMAIUS_GUI_PANEL_CONTROL_H
#define DROMAIUS_GUI_PANEL_CONTROL_H

#include <initializer_list>

#include "panel.h"
#include "signal_line.h"

struct StepSignal {
	Signal	signal;
	bool	pos_edge;
	bool	neg_edge;
};

[[maybe_unused]] constexpr StepSignal STEP_SIGNAL_NONE = {SIGNAL_INIT_UNDEFINED, false, false};

// functions
Panel::uptr_t panel_control_create( class UIContext *ctx, struct ImVec2 pos,
									struct Oscillator *oscillator,
									StepSignal step_next_instruction,
									std::initializer_list<StepSignal> step_clocks
);

#endif // DROMAIUS_GUI_PANEL_CONTROL_H
