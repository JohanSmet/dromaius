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

#define SIGNAL_POOL			pia->signal_pool
#define SIGNAL_COLLECTION	pia->signals

class PanelChip6520 : public Panel {
public:
	PanelChip6520(UIContext *ctx, ImVec2 pos, Chip6520 *pia) :
		Panel(ctx),
		position(pos),
		pia(pia) {
	}

	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
		
		if (ImGui::Begin(title)) {
			auto origin = ImGui::GetCursorPos();

			// left column
			ui_register_8bit(8, "Periph. Output A", pia->reg_ora);
			ui_register_8bit_binary(8, "Data Direction A", pia->reg_ddra);
			ui_register_8bit_binary(8, "Control Reg. A", pia->reg_cra.reg);

			ui_register_8bit(8, "Periph. Output B", pia->reg_orb);
			ui_register_8bit_binary(8, "Data Direction B", pia->reg_ddrb);
			ui_register_8bit_binary(8, "Control Reg. B", pia->reg_crb.reg);

			ui_register_8bit(8, "Port-A", SIGNAL_NEXT_UINT8(port_a));
			ui_register_8bit(8, "Port-B", SIGNAL_NEXT_UINT8(port_b));

			// middle column
			ImGui::SetCursorPos(origin);
			ui_signal(220, "CA1", SIGNAL_NEXT_BOOL(ca1), ACTHI_ASSERT);
			ui_signal(220, "CA2", SIGNAL_NEXT_BOOL(ca2), ACTHI_ASSERT);
			ui_signal(220, "/IRQA", SIGNAL_NEXT_BOOL(irqa_b), ACTLO_ASSERT);

			ui_signal(220, "CS0", SIGNAL_NEXT_BOOL(cs0), ACTHI_ASSERT);
			ui_signal(220, "CS1", SIGNAL_NEXT_BOOL(cs1), ACTHI_ASSERT);
			ui_signal(220, "/CS2", SIGNAL_NEXT_BOOL(cs2_b), ACTLO_ASSERT);

			// right column
			ImGui::SetCursorPos(origin);
			ui_signal(330, "CB1", SIGNAL_NEXT_BOOL(ca1), ACTHI_ASSERT);
			ui_signal(330, "CB2", SIGNAL_NEXT_BOOL(ca2), ACTHI_ASSERT);
			ui_signal(330, "/IRQB", SIGNAL_NEXT_BOOL(irqa_b), ACTLO_ASSERT);

			ui_signal(330, "RS0", SIGNAL_NEXT_BOOL(rs0), ACTHI_ASSERT);
			ui_signal(330, "RS1", SIGNAL_NEXT_BOOL(rs1), ACTHI_ASSERT);
			ui_signal(330, "/RES", SIGNAL_NEXT_BOOL(reset_b), ACTLO_ASSERT);
		}

		ImGui::End();
	}

private:
	ImVec2				position;
	const ImVec2		size = {460, 0};

	Chip6520 *			pia;
	
	constexpr static const char *title = "PIA - 6520";

};


Panel::uptr_t panel_chip_6520_create(UIContext *ctx, struct ImVec2 pos, struct Chip6520 *pia) {
	return std::make_unique<PanelChip6520>(ctx, pos, pia);
}

