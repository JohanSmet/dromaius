// gui/panel_cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a MOS 6502

#include "panel_cpu_6502.h"

#include "widgets.h"
#include "cpu_6502.h"

#define SIGNAL_POOL			cpu->signal_pool
#define SIGNAL_COLLECTION	cpu->signals

void panel_cpu_6502(struct nk_context *nk_ctx, struct nk_vec2 pos, struct Cpu6502 *cpu) {

	static const char *panel_title = "CPU - MOS 6502";
	const struct nk_rect panel_bounds = {
		.x = pos.x,
		.y = pos.y,
		.w = 410,
		.h = 220
	};
	static const nk_flags panel_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR;

	if (nk_begin(nk_ctx, panel_title, panel_bounds, panel_flags)) {

		nk_layout_row_dynamic(nk_ctx, 180, 2);
			
		if (nk_group_begin(nk_ctx, "col_left", 0)) {
		
			ui_register_8bit(nk_ctx, "Accumulator", cpu->reg_a);
			ui_register_8bit(nk_ctx, "index-X", cpu->reg_x);
			ui_register_8bit(nk_ctx, "index-Y", cpu->reg_y);
			ui_register_8bit(nk_ctx, "Stack Pointer", cpu->reg_sp);
			ui_register_16bit(nk_ctx, "Program Counter", cpu->reg_pc);
			ui_register_8bit(nk_ctx, "Instruction", cpu->reg_ir);
			ui_register_8bit(nk_ctx, "Processor Status", cpu->reg_p);
			ui_register_16bit(nk_ctx, "Address Bus", SIGNAL_NEXT_UINT16(bus_address));
			ui_register_8bit(nk_ctx, "Data Bus", SIGNAL_NEXT_UINT8(bus_data));

			nk_group_end(nk_ctx);
		}
		
		if (nk_group_begin(nk_ctx, "col_right", 0)) {

			nk_layout_row_static(nk_ctx, 12, 10, 8);
			nk_label(nk_ctx, "N", NK_TEXT_CENTERED);
			nk_label(nk_ctx, "V", NK_TEXT_CENTERED);
			nk_label(nk_ctx, "-", NK_TEXT_CENTERED);
			nk_label(nk_ctx, "B", NK_TEXT_CENTERED);
			nk_label(nk_ctx, "D", NK_TEXT_CENTERED);
			nk_label(nk_ctx, "I", NK_TEXT_CENTERED);
			nk_label(nk_ctx, "Z", NK_TEXT_CENTERED);
			nk_label(nk_ctx, "C", NK_TEXT_CENTERED);

			nk_layout_row_static(nk_ctx, 12, 10, 8);
			nk_labelf(nk_ctx, NK_TEXT_CENTERED, "%d", cpu->p_negative_result);
			nk_labelf(nk_ctx, NK_TEXT_CENTERED, "%d", cpu->p_overflow);
			nk_labelf(nk_ctx, NK_TEXT_CENTERED, "%d", cpu->p_expension);
			nk_labelf(nk_ctx, NK_TEXT_CENTERED, "%d", cpu->p_break_command);
			nk_labelf(nk_ctx, NK_TEXT_CENTERED, "%d", cpu->p_decimal_mode);
			nk_labelf(nk_ctx, NK_TEXT_CENTERED, "%d", cpu->p_interrupt_disable);
			nk_labelf(nk_ctx, NK_TEXT_CENTERED, "%d", cpu->p_zero_result);
			nk_labelf(nk_ctx, NK_TEXT_CENTERED, "%d", cpu->p_carry);

			ui_signal(nk_ctx, "/RES", SIGNAL_NEXT_BOOL(reset_b), ACTLO_ASSERT);
			ui_signal(nk_ctx, "/IRQ", SIGNAL_NEXT_BOOL(irq_b), ACTLO_ASSERT);
			ui_signal(nk_ctx, "/NMI", SIGNAL_NEXT_BOOL(nmi_b), ACTLO_ASSERT);
			ui_signal(nk_ctx, "RDY",  SIGNAL_NEXT_BOOL(rdy), ACTHI_ASSERT);
			ui_signal(nk_ctx, "SYNC", SIGNAL_NEXT_BOOL(sync), ACTHI_ASSERT);
			ui_signal(nk_ctx, "R/W",  SIGNAL_NEXT_BOOL(rw), ACTHI_ASSERT);

			nk_group_end(nk_ctx);
		}
	}
	nk_end(nk_ctx);

}
