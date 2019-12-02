// gui/panel_cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a MOS 6502

#include "panel_cpu_6502.h"

#include <nuklear/nuklear_std.h>
#include "cpu_6502.h"
#include "nuklear/nuklear.h"

static inline void ui_cpu_register_8bit(struct nk_context *nk_ctx, const char *name, uint8_t value) {
	nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
	nk_layout_row_push(nk_ctx, 128);
	nk_labelf(nk_ctx, NK_TEXT_RIGHT, "%s: ", name);
	nk_layout_row_push(nk_ctx, 32);
	nk_labelf(nk_ctx, NK_TEXT_LEFT, "$%.2x", value);
}

static inline void ui_cpu_register_16bit(struct nk_context *nk_ctx, const char *name, uint16_t value) {
	nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
	nk_layout_row_push(nk_ctx, 128);
	nk_labelf(nk_ctx, NK_TEXT_RIGHT, "%s: ", name);
	nk_layout_row_push(nk_ctx, 32);
	nk_labelf(nk_ctx, NK_TEXT_LEFT, "$%.4x", value);
}

static inline void ui_cpu_signal(struct nk_context *nk_ctx, const char *name, bool value, bool asserted) {
	static const char *STR_VALUE[] = {"Low", "High"};
	const struct nk_color SIG_COLOR[] = {{200, 0, 0, 255}, {0, 200, 0, 255}};

	nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
	nk_layout_row_push(nk_ctx, 48);
	nk_labelf(nk_ctx, NK_TEXT_LEFT, "%s: ", name);
	nk_layout_row_push(nk_ctx, 32);
	nk_label_colored(nk_ctx, STR_VALUE[value], NK_TEXT_LEFT, SIG_COLOR[asserted]);
}

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
		
			ui_cpu_register_8bit(nk_ctx, "Accumulator", cpu->reg_a);
			ui_cpu_register_8bit(nk_ctx, "index-X", cpu->reg_x);
			ui_cpu_register_8bit(nk_ctx, "index-Y", cpu->reg_y);
			ui_cpu_register_8bit(nk_ctx, "Stack Pointer", cpu->reg_sp);
			ui_cpu_register_16bit(nk_ctx, "Program Counter", cpu->reg_pc);
			ui_cpu_register_8bit(nk_ctx, "Instruction", cpu->reg_ir);
			ui_cpu_register_8bit(nk_ctx, "Processor Status", cpu->reg_p);
			ui_cpu_register_16bit(nk_ctx, "Address Bus", *cpu->bus_address);
			ui_cpu_register_8bit(nk_ctx, "Data Bus", *cpu->bus_data);

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

			ui_cpu_signal(nk_ctx, "/RES", *cpu->pin_reset_b, ACTLO_ASSERTED(*cpu->pin_reset_b));
			ui_cpu_signal(nk_ctx, "/IRQ", *cpu->pin_irq_b, ACTLO_ASSERTED(*cpu->pin_irq_b));
			ui_cpu_signal(nk_ctx, "/NMI", *cpu->pin_nmi_b, ACTLO_ASSERTED(*cpu->pin_nmi_b));
			ui_cpu_signal(nk_ctx, "RDY", *cpu->pin_rdy, ACTHI_ASSERTED(*cpu->pin_rdy));
			ui_cpu_signal(nk_ctx, "SYNC", *cpu->pin_sync, ACTHI_ASSERTED(*cpu->pin_sync));
			ui_cpu_signal(nk_ctx, "R/W", *cpu->pin_rw, ACTHI_ASSERTED(*cpu->pin_rw));

			nk_group_end(nk_ctx);
		}
	}
	nk_end(nk_ctx);

}
