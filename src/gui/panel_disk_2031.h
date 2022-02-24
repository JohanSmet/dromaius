// gui/panel_disk_2031.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_PANEL_DISK_2031_H
#define DROMAIUS_GUI_PANEL_DISK_2031_H

#include "panel.h"

// functions
Panel::uptr_t panel_fd2031_create(	class UIContext *ctx, struct ImVec2 pos,
									struct PerifDisk2031 *ieee_tester
);

#endif // DROMAIUS_GUI_PANEL_DISK_2031_H
