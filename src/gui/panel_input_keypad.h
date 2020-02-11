// gui/panel_input_keypad.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_PANEL_INPUT_KEYPAD_H
#define DROMAIUS_GUI_PANEL_INPUT_KEYPAD_H

#include "panel.h"

Panel::uptr_t panel_input_keypad_create(class UIContext *ctx, struct ImVec2 pos, struct InputKeypad *keypad);

#endif // DROMAIUS_GUI_PANEL_INPUT_KEYPAD_H
