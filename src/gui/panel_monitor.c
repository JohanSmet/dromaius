// gui/panel_monitor.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// a simple hardware monitor

#include "panel_monitor.h"
#include "context.h"
#include "utils.h"

#include <nuklear/nuklear_std.h>
#include <stb/stb_ds.h>
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

typedef struct PanelMonitor {
	struct nk_context *		nk_ctx;
	struct DmsContext *		dms_ctx;
	struct nk_rect			bounds;

	char **					output;
	char					input[256];
} PanelMonitor;

///////////////////////////////////////////////////////////////////////////////
//
// private functions
//

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

PanelMonitor *panel_monitor_init(struct nk_context *nk_ctx, struct nk_vec2 pos, struct DmsContext *dms_ctx) {
	
	PanelMonitor *pnl = (PanelMonitor *) malloc(sizeof(PanelMonitor));
	pnl->nk_ctx = nk_ctx;
	pnl->dms_ctx = dms_ctx;
	pnl->bounds = nk_rect(pos.x, pos.y, 400, 400);

	pnl->output = NULL;
	pnl->input[0] = '\0';

	return pnl;
}

void panel_monitor_release(struct PanelMonitor *pnl) {
	assert(pnl);
	
	for (int i = 0; i < arrlen(pnl->output); ++i) {
		arrfree(pnl->output[i]);
	}
	arrfree(pnl->output);
	free(pnl);
}

void panel_monitor_display(struct PanelMonitor *pnl) {
	static const char *panel_title = "Monitor";
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR;

	if (nk_begin(pnl->nk_ctx, panel_title, pnl->bounds, panel_flags)) {
		// previous input + output
		nk_layout_row_static(pnl->nk_ctx, 320, 380, 1);

		if (nk_group_begin(pnl->nk_ctx, "output", NK_WINDOW_BORDER)) {
			for (int i = 0; i < arrlen(pnl->output); ++i) {
				nk_layout_row_static(pnl->nk_ctx, 12, 355, 1);
				nk_label(pnl->nk_ctx, pnl->output[i], NK_TEXT_LEFT);
			}
			
			nk_group_end(pnl->nk_ctx);
		}
		
		// input
		nk_layout_row_static(pnl->nk_ctx, 25, 380, 1);
		nk_flags event = nk_edit_string_zero_terminated(pnl->nk_ctx,
							NK_EDIT_BOX | NK_EDIT_SIG_ENTER,
							pnl->input, sizeof(pnl->input), nk_filter_ascii);
		
		if (event & NK_EDIT_COMMITED && strlen(pnl->input) > 0) {
			arrpush(pnl->output, NULL);
			arr_printf(arrlast(pnl->output), "%s", pnl->input);
			char *reply = NULL;
			dms_monitor_cmd(pnl->dms_ctx, pnl->input, &reply);
			pnl->input[0] = '\0';
			arrpush(pnl->output, reply);
		}
	}
	nk_end(pnl->nk_ctx);
}
