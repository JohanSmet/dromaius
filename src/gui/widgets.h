// gui/widgets.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// functions for widgets used in multiple panels

#ifndef DROMAIUS_GUI_WIDGETS_H
#define DROMAIUS_GUI_WIDGETS_H

#include <nuklear/nuklear_std.h>
#include <stdint.h>
#include <stdbool.h>

static const char *nibble_bit_string[16] = {
	[ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
	[ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
	[ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
	[12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111"
};

static inline void ui_register_8bit(struct nk_context *nk_ctx, const char *name, uint8_t value) {
	nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
	nk_layout_row_push(nk_ctx, 128);
	nk_labelf(nk_ctx, NK_TEXT_RIGHT, "%s: ", name);
	nk_layout_row_push(nk_ctx, 32);
	nk_labelf(nk_ctx, NK_TEXT_LEFT, "$%.2x", value);
}

static inline void ui_register_8bit_binary(struct nk_context *nk_ctx, const char *name, uint8_t value) {
	nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
	nk_layout_row_push(nk_ctx, 128);
	nk_labelf(nk_ctx, NK_TEXT_RIGHT, "%s: ", name);
	nk_layout_row_push(nk_ctx, 64);
	nk_labelf(nk_ctx, NK_TEXT_LEFT, "%%%s%s", nibble_bit_string[value >> 4], nibble_bit_string[value & 0xf]);
}

static inline void ui_register_16bit(struct nk_context *nk_ctx, const char *name, uint16_t value) {
	nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
	nk_layout_row_push(nk_ctx, 128);
	nk_labelf(nk_ctx, NK_TEXT_RIGHT, "%s: ", name);
	nk_layout_row_push(nk_ctx, 32);
	nk_labelf(nk_ctx, NK_TEXT_LEFT, "$%.4x", value);
}

static inline void ui_signal(struct nk_context *nk_ctx, const char *name, bool value, bool asserted) {
	static const char *STR_VALUE[] = {"Low", "High"};
	const struct nk_color SIG_COLOR[] = {{200, 0, 0, 255}, {0, 200, 0, 255}};

	nk_layout_row_begin(nk_ctx, NK_STATIC, 14, 2);
	nk_layout_row_push(nk_ctx, 48);
	nk_labelf(nk_ctx, NK_TEXT_LEFT, "%s: ", name);
	nk_layout_row_push(nk_ctx, 32);
	nk_label_colored(nk_ctx, STR_VALUE[value], NK_TEXT_LEFT, SIG_COLOR[value == asserted]);
}

#endif // DROMAIUS_GUI_WIDGETS_H