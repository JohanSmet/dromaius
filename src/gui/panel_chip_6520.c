// gui/panel_chip_6520.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a 6520 Peripheral Interace Adapter

#include "panel_chip_6520.h"
#include "chip_6520.h"
#include "context.h"
#include "utils.h"
#include "widgets.h"

#include <stb/stb_ds.h>
#include <assert.h>

#define SIGNAL_POOL			pnl->pia->signal_pool
#define SIGNAL_COLLECTION	pnl->pia->signals

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

typedef struct PanelChip6520 {
	struct nk_context *		nk_ctx;
	struct DmsContext *		dms_ctx;
	struct nk_rect			bounds;

	Chip6520 *				pia;
} PanelChip6520;

///////////////////////////////////////////////////////////////////////////////
//
// private functions
//

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

PanelChip6520 *panel_chip_6520_init(struct nk_context *nk_ctx, struct nk_vec2 pos, struct DmsContext *dms_ctx, struct Chip6520 *pia) {

	PanelChip6520 *pnl = (PanelChip6520 *) malloc(sizeof(PanelChip6520));
	pnl->nk_ctx = nk_ctx;
	pnl->dms_ctx = dms_ctx;
	pnl->bounds = nk_rect(pos.x, pos.y, 480, 200);
	pnl->pia = pia;

	return pnl;
}

void panel_chip_6520_release(struct PanelChip6520 *pnl) {
	assert(pnl);
	free(pnl);
}

void panel_chip_6520_display(struct PanelChip6520 *pnl) {
	static const char *panel_title = "PIA - 6520";
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR;

	if (nk_begin(pnl->nk_ctx, panel_title, pnl->bounds, panel_flags)) {
		nk_layout_row(pnl->nk_ctx, NK_STATIC, 180, 3, (float[]) {220, 110, 110});

		if (nk_group_begin(pnl->nk_ctx, "col_left", 0)) {
			ui_register_8bit(pnl->nk_ctx, "Periph. Output A", pnl->pia->reg_ora);
			ui_register_8bit_binary(pnl->nk_ctx, "Data Direction A", pnl->pia->reg_ddra);
			ui_register_8bit_binary(pnl->nk_ctx, "Control Reg. A", pnl->pia->reg_cra.reg);

			ui_register_8bit(pnl->nk_ctx, "Periph. Output B", pnl->pia->reg_orb);
			ui_register_8bit_binary(pnl->nk_ctx, "Data Direction B", pnl->pia->reg_ddrb);
			ui_register_8bit_binary(pnl->nk_ctx, "Control Reg. b", pnl->pia->reg_crb.reg);

			ui_register_8bit(pnl->nk_ctx, "Port-A", SIGNAL_NEXT_UINT8(port_a));
			ui_register_8bit(pnl->nk_ctx, "Port-B", SIGNAL_NEXT_UINT8(port_b));

			nk_group_end(pnl->nk_ctx);
		}

		if (nk_group_begin(pnl->nk_ctx, "col_middle", 0)) {
			ui_signal(pnl->nk_ctx, "CA1", SIGNAL_NEXT_BOOL(ca1), ACTHI_ASSERT);
			ui_signal(pnl->nk_ctx, "CA2", SIGNAL_NEXT_BOOL(ca2), ACTHI_ASSERT);
			ui_signal(pnl->nk_ctx, "/IRQA", SIGNAL_NEXT_BOOL(irqa_b), ACTLO_ASSERT);

			ui_signal(pnl->nk_ctx, "CS0", SIGNAL_NEXT_BOOL(cs0), ACTHI_ASSERT);
			ui_signal(pnl->nk_ctx, "CS1", SIGNAL_NEXT_BOOL(cs1), ACTHI_ASSERT);
			ui_signal(pnl->nk_ctx, "/CS2", SIGNAL_NEXT_BOOL(cs2_b), ACTLO_ASSERT);

			nk_group_end(pnl->nk_ctx);
		}

		if (nk_group_begin(pnl->nk_ctx, "col_right", 0)) {
			ui_signal(pnl->nk_ctx, "CB1", SIGNAL_NEXT_BOOL(ca1), ACTHI_ASSERT);
			ui_signal(pnl->nk_ctx, "CB2", SIGNAL_NEXT_BOOL(ca2), ACTHI_ASSERT);
			ui_signal(pnl->nk_ctx, "/IRQB", SIGNAL_NEXT_BOOL(irqa_b), ACTLO_ASSERT);

			ui_signal(pnl->nk_ctx, "RS0", SIGNAL_NEXT_BOOL(rs0), ACTHI_ASSERT);
			ui_signal(pnl->nk_ctx, "RS1", SIGNAL_NEXT_BOOL(rs1), ACTHI_ASSERT);
			ui_signal(pnl->nk_ctx, "/RES", SIGNAL_NEXT_BOOL(reset_b), ACTLO_ASSERT);

			nk_group_end(pnl->nk_ctx);
		}

	}
	nk_end(pnl->nk_ctx);
}
