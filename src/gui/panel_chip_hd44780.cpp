// gui/panel_chip_hd44780.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display the LCD output of a Hitachi HD44780 LCD Controller/Driver

#include "panel_chip_hd44780.h"
#include "chip_hd44780.h"
#include "context.h"
#include "widgets.h"

#define SIGNAL_PREFIX		CHIP_HD44780_
#define SIGNAL_OWNER		lcd

inline ImVec2 operator+(const ImVec2 &a, const ImVec2 &b) {
	return {a.x + b.x, a.y + b.y};
}

class PanelChipHd44780 : public Panel {
public:

	PanelChipHd44780(UIContext *ctx, ImVec2 pos, ChipHd44780 *lcd) :
		Panel(ctx),
		position(pos),
		lcd(lcd) {
	}

	void display() override {

		constexpr ImU32 lcd_colors[2] = { IM_COL32(80, 80, 90, 255), IM_COL32(220, 220, 250, 255) };

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);

		auto cell_size = ImVec2(4,4);
		auto spacing = ImVec2(1, 1);
		auto lcd_size = ImVec2(
				static_cast<float>(lcd->display_width) * lcd->char_width * (cell_size.x + spacing.x) + ((lcd->display_width + 2.0f) * spacing.x * 2.0f),
				static_cast<float>(lcd->display_height) * lcd->char_height * (cell_size.y + spacing.y) + ((lcd->display_height + 2.0f) * spacing.y * 2.0f));

		ImGui::SetNextWindowSizeConstraints(lcd_size + ImVec2(16, 56), ImVec2(1024, 1024));

		if (ImGui::Begin(title)) {
			auto draw_list = ImGui::GetWindowDrawList();

			auto lcd_origin = ImGui::GetCursorScreenPos();

			// >> background
			draw_list->AddRectFilled(lcd_origin, lcd_origin + lcd_size, IM_COL32(0, 0, 200, 255));

			// >> lcd cells
			auto origin = lcd_origin + cell_size;
			uint8_t *pixel = lcd->display_data;

			for (int l = 0; l < lcd->display_height * lcd->char_height; ++l) {
				for (int c = 0; c < lcd->display_width * lcd->char_width; ++c) {
					draw_list->AddRectFilled(origin, origin + cell_size, lcd_colors[(*pixel++)]);
					origin.x += cell_size.x + spacing.x;

					if ((c+1) % lcd->char_width == 0) {
						origin.x += spacing.x * 2;
					}
				}

				origin.x = lcd_origin.x + cell_size.x;
				origin.y += cell_size.y + spacing.y;

				if ((l+1) % lcd->char_height == 0) {
					origin.y += spacing.y * 2;
				}
			}

			// signals
			ImGui::SetCursorScreenPos({ImGui::GetCursorScreenPos().x, lcd_origin.y + lcd_size.y + 4});
			ui_signal(0, "RS", SIGNAL_READ_NEXT(RS), ACTHI_ASSERT);
			ImGui::SameLine();
			ui_signal(150, "RW", SIGNAL_READ_NEXT(RW), ACTHI_ASSERT);
			ImGui::SameLine();
			ui_signal(300, "E", SIGNAL_READ_NEXT(E), ACTHI_ASSERT);
		}

		ImGui::End();
	}

private:
	ImVec2				position;
	const ImVec2		size = {480, 0};
	ChipHd44780 *		lcd;

	constexpr static const char *title = "LCD - HD44780";
};

Panel::uptr_t panel_chip_hd44780_create(UIContext *ctx, struct ImVec2 pos, struct ChipHd44780 *lcd) {
	return std::make_unique<PanelChipHd44780>(ctx, pos, lcd);
}


/*

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
	pnl->bounds = nk_rect(pos.x, pos.y, 480, 150);
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
	static const struct nk_color lcd_colors[2] = { {80, 80, 90, 255}, {220, 220, 250, 255}};

	if (nk_begin(pnl->nk_ctx, panel_title, pnl->bounds, panel_flags)) {

		// lcd output
		nk_layout_row(pnl->nk_ctx, NK_STATIC, 4 + ((2 + (5 * pnl->lcd->char_height)) * pnl->lcd->display_height), 1, (float[]) {436});
		struct nk_rect tl;
		nk_widget(&tl, pnl->nk_ctx);
		struct nk_command_buffer *canvas = nk_window_get_canvas(pnl->nk_ctx);

		// >> background
		nk_fill_rect(canvas, tl, 0.0f, nk_rgb(0, 0, 200));
		tl.x += 4;
		tl.y += 4;

		// >> lcd cells
		struct nk_rect cell = {tl.x, tl.y, 4, 4};
		uint8_t *pixel = pnl->lcd->display_data;

		for (int l = 0; l < pnl->lcd->display_height * pnl->lcd->char_height; ++l) {
			for (int c = 0; c < pnl->lcd->display_width * pnl->lcd->char_width; ++c) {
				nk_fill_rect(canvas, cell, false, lcd_colors[(*pixel++)]);
				cell.x += 5;
				if ((c+1) % pnl->lcd->char_width == 0) {
					cell.x += 2;
				}
			}

			cell.x = tl.x;
			cell.y += 5;
			if ((l+1) % pnl->lcd->char_height == 0) {
				cell.y += 2;
			}
		}

		// signals
		nk_layout_row(pnl->nk_ctx, NK_STATIC, 450, 3, (float[]) {150,150,150});
		if (nk_group_begin(pnl->nk_ctx, "col_left", 0)) {
			ui_signal(pnl->nk_ctx, "RS", SIGNAL_READ_NEXT(rs), ACTHI_ASSERT);
			nk_group_end(pnl->nk_ctx);
		}
		if (nk_group_begin(pnl->nk_ctx, "col_middle", 0)) {
			ui_signal(pnl->nk_ctx, "RW", SIGNAL_READ_NEXT(rw), ACTHI_ASSERT);
			nk_group_end(pnl->nk_ctx);
		}
		if (nk_group_begin(pnl->nk_ctx, "col_right", 0)) {
			ui_signal(pnl->nk_ctx, "E", SIGNAL_READ_NEXT(enable), ACTHI_ASSERT);
			nk_group_end(pnl->nk_ctx);
		}
	}
	nk_end(pnl->nk_ctx);

	int height = 65 + (pnl->lcd->display_height * 60);
	if (height != pnl->bounds.h) {
		nk_window_set_size(pnl->nk_ctx, panel_title, nk_vec2(pnl->bounds.w, height));
		pnl->bounds.h = height;
	}
}

*/
