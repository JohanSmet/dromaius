// gui/panel_chip_6520.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a 6520 Peripheral Interace Adapter

#define SIGNAL_ARRAY_STYLE
#include "panel_chip_6520.h"
#include "chip_6520.h"
#include "context.h"
#include "utils.h"
#include "widgets.h"
#include "ui_context.h"

#include <stb/stb_ds.h>
#include <assert.h>

#define SIGNAL_PREFIX		CHIP_6520_
#define SIGNAL_OWNER		pia

class PanelChip6520 : public Panel {
public:
	PanelChip6520(UIContext *ctx, ImVec2 pos, Chip6520 *pia) :
		Panel(ctx),
		position(pos),
		pia(pia) {
		title = ui_context->unique_panel_id("PIA - MOS 6520");
	}

	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {
			auto origin = ImGui::GetCursorPos();

			// left column
			ui_register_8bit(8, "Periph. Output A", pia->reg_ora);
			ui_register_8bit_binary(8, "Data Direction A", pia->reg_ddra);
			ui_register_8bit_binary(8, "Control Reg. A", pia->reg_cra);

			ui_register_8bit(8, "Periph. Output B", pia->reg_orb);
			ui_register_8bit_binary(8, "Data Direction B", pia->reg_ddrb);
			ui_register_8bit_binary(8, "Control Reg. B", pia->reg_crb);

			ui_register_8bit(8, "Port-A", SIGNAL_GROUP_READ_NEXT_U8(port_a));
			ui_register_8bit(8, "Port-B", SIGNAL_GROUP_READ_NEXT_U8(port_b));

			// middle column
			ImGui::SetCursorPos(origin);
			ui_signal(220, "CA1", SIGNAL_READ_NEXT(CA1), ACTHI_ASSERT);
			ui_signal(220, "CA2", SIGNAL_READ_NEXT(CA2), ACTHI_ASSERT);
			ui_signal(220, "/IRQA", SIGNAL_READ_NEXT(IRQA_B), ACTLO_ASSERT);

			ui_signal(220, "CS0", SIGNAL_READ_NEXT(CS0), ACTHI_ASSERT);
			ui_signal(220, "CS1", SIGNAL_READ_NEXT(CS1), ACTHI_ASSERT);
			ui_signal(220, "/CS2", SIGNAL_READ_NEXT(CS2_B), ACTLO_ASSERT);

			// right column
			ImGui::SetCursorPos(origin);
			ui_signal(330, "CB1", SIGNAL_READ_NEXT(CB1), ACTHI_ASSERT);
			ui_signal(330, "CB2", SIGNAL_READ_NEXT(CB2), ACTHI_ASSERT);
			ui_signal(330, "/IRQB", SIGNAL_READ_NEXT(IRQB_B), ACTLO_ASSERT);

			ui_signal(330, "RS0", SIGNAL_READ_NEXT(RS0), ACTHI_ASSERT);
			ui_signal(330, "RS1", SIGNAL_READ_NEXT(RS1), ACTHI_ASSERT);
			ui_signal(330, "/RES", SIGNAL_READ_NEXT(RESET_B), ACTLO_ASSERT);
		}

		ImGui::End();
	}

private:
	ImVec2				position;
	const ImVec2		size = {460, 0};
	std::string			title;

	Chip6520 *			pia;
};


Panel::uptr_t panel_chip_6520_create(UIContext *ctx, struct ImVec2 pos, struct Chip6520 *pia) {
	return std::make_unique<PanelChip6520>(ctx, pos, pia);
}

