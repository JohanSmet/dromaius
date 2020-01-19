// gui/panel_chip_hd44780.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display the LCD output of a Hitachi HD44780 LCD Controller/Driver

#include "panel_chip_hd44780.h"
#include "chip_hd44780.h"
#include "context.h"
#include "utils.h"
#include "widgets.h"

#include <assert.h>
#include <stdlib.h>

#define SIGNAL_POOL			pnl->lcd->signal_pool
#define SIGNAL_COLLECTION	pnl->lcd->signals

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

typedef struct PanelChipHd44780 {
	struct nk_context *		nk_ctx;
	struct DmsContext *		dms_ctx;
	struct nk_rect			bounds;

	ChipHd44780 *			lcd;
} PanelChipHd44780;

///////////////////////////////////////////////////////////////////////////////
//
// private functions
//

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

PanelChipHd44780 *panel_chip_hd44780_init(struct nk_context *nk_ctx, struct nk_vec2 pos, struct DmsContext *dms_ctx, struct ChipHd44780 *lcd) {

	PanelChipHd44780 *pnl = (PanelChipHd44780 *) malloc(sizeof(PanelChipHd44780));
	pnl->nk_ctx = nk_ctx;
	pnl->dms_ctx = dms_ctx;
	pnl->bounds = nk_rect(pos.x, pos.y, 480, 200);
	pnl->lcd = lcd;

	return pnl;
}

void panel_chip_hd44780_release(struct PanelChipHd44780 *pnl) {
	assert(pnl);
	free(pnl);
}

void panel_chip_hd44780_display(struct PanelChipHd44780 *pnl) {
	static const char *panel_title = "LCD - HD44780";
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR;

	if (nk_begin(pnl->nk_ctx, panel_title, pnl->bounds, panel_flags)) {
		nk_layout_row(pnl->nk_ctx, NK_STATIC, 30, 1, (float[]) {200});
		if (pnl->lcd->display_enabled) {
			nk_label(pnl->nk_ctx, (const char *) pnl->lcd->ddram, NK_TEXT_LEFT);
		}
		ui_signal(pnl->nk_ctx, "RS", SIGNAL_NEXT_BOOL(rs), ACTHI_ASSERT);
		ui_signal(pnl->nk_ctx, "RW", SIGNAL_NEXT_BOOL(rw), ACTHI_ASSERT);
		ui_signal(pnl->nk_ctx, "E", SIGNAL_NEXT_BOOL(enable), ACTHI_ASSERT);
	}
	nk_end(pnl->nk_ctx);
}
