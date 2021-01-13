// gui/panel_chip_6522.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a 6522 Peripheral Interace Adapter

#define SIGNAL_ARRAY_STYLE
#include "panel_chip_6522.h"
#include "chip_6522.h"
#include "context.h"
#include "utils.h"
#include "widgets.h"
#include "ui_context.h"

#include <stb/stb_ds.h>
#include <assert.h>

#define SIGNAL_PREFIX		CHIP_6522_
#define SIGNAL_OWNER		via

class PanelChip6522 : public Panel {
public:
	PanelChip6522(UIContext *ctx, ImVec2 pos, Chip6522 *via) :
		Panel(ctx),
		position(pos),
		via(via) {
		title = ui_context->unique_panel_id("PIA - MOS 6522");
	}

	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {
			auto origin = ImGui::GetCursorPos();

			// left column
			ui_register_8bit_binary(8, "Interrupt Flags", via->reg_ifr);
			ui_register_8bit_binary(8, "Interrupt Enable", via->reg_ier);

			ui_register_8bit_binary(8, "Periph. Control", via->reg_pcr);
			ui_register_8bit_binary(8, "Auxiliary Control", via->reg_acr);

			ui_register_8bit(8,  "T1 -- low latch", via->reg_t1l_l);
			ui_register_8bit(8,  "T1 - high latch", via->reg_t1l_h);
			ui_register_16bit(8, "T1 ---- counter", via->reg_t1c);

			ui_register_8bit(8,  "T2 --- latch", via->reg_t2l_l);
			ui_register_16bit(8, "T2 - counter", via->reg_t2c);

			ui_register_8bit(8, "Shift Register", via->reg_sr);

			ui_register_8bit(8, "Input Latch A", via->reg_ila);
			ui_register_8bit(8, "Output reg A", via->reg_ora);
			ui_register_8bit_binary(8, "Data Direction A", via->reg_ddra);

			ui_register_8bit(8, "Input Latch B", via->reg_ilb);
			ui_register_8bit(8, "Output reg B", via->reg_orb);
			ui_register_8bit_binary(8, "Data Direction B", via->reg_ddrb);

			ui_register_8bit(8, "Port-A", SIGNAL_GROUP_READ_NEXT_U8(port_a));
			ui_register_8bit(8, "Port-B", SIGNAL_GROUP_READ_NEXT_U8(port_b));

			// middle column
			ImGui::SetCursorPos(origin);

			ui_signal(220, "CS1", SIGNAL_READ_NEXT(CS1), ACTHI_ASSERT);
			ui_signal(220, "/CS2", SIGNAL_READ_NEXT(CS2_B), ACTLO_ASSERT);
			ui_signal(220, "RS0", SIGNAL_READ_NEXT(RS0), ACTHI_ASSERT);
			ui_signal(220, "RS1", SIGNAL_READ_NEXT(RS1), ACTHI_ASSERT);
			ui_signal(220, "RS2", SIGNAL_READ_NEXT(RS2), ACTHI_ASSERT);
			ui_signal(220, "RS3", SIGNAL_READ_NEXT(RS3), ACTHI_ASSERT);

			// right column
			ImGui::SetCursorPos(origin);
			ui_signal(330, "CA1", SIGNAL_READ_NEXT(CA1), ACTHI_ASSERT);
			ui_signal(330, "CA2", SIGNAL_READ_NEXT(CA2), ACTHI_ASSERT);
			ui_signal(330, "CB1", SIGNAL_READ_NEXT(CA1), ACTHI_ASSERT);
			ui_signal(330, "CB2", SIGNAL_READ_NEXT(CA2), ACTHI_ASSERT);
			ui_signal(330, "/IRQ", SIGNAL_READ_NEXT(IRQ_B), ACTLO_ASSERT);

			ui_signal(330, "/RES", SIGNAL_READ_NEXT(RESET_B), ACTLO_ASSERT);
		}

		ImGui::End();
	}

private:
	ImVec2				position;
	const ImVec2		size = {460, 0};
	std::string			title;

	Chip6522 *			via;

};


Panel::uptr_t panel_chip_6522_create(UIContext *ctx, struct ImVec2 pos, struct Chip6522 *via) {
	return std::make_unique<PanelChip6522>(ctx, pos, via);
}

