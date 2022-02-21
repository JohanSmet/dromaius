// gui/panel_chip_6520.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a 6520 Peripheral Interace Adapter

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

		SignalValue sig_values[8];

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {
			if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen)) {
				ui_register_8bit(8, "Periph. Output A", pia->reg_ora);
				ui_register_8bit_binary(8, "Data Direction A", pia->reg_ddra);
				ui_register_8bit_binary(8, "Control Reg. A", pia->reg_cra);
				ImGui::Separator();
				ui_register_8bit(8, "Periph. Output B", pia->reg_orb);
				ui_register_8bit_binary(8, "Data Direction B", pia->reg_ddrb);
				ui_register_8bit_binary(8, "Control Reg. B", pia->reg_crb);
			}

			if (ImGui::CollapsingHeader("Ports", ImGuiTreeNodeFlags_DefaultOpen)) {
				ui_signal_bits(8, "Port-A  In", 128, SIGNAL_GROUP_VALUE(port_a, sig_values), 8);
				ui_signal_bits(8, "Port-A Out", 128, SIGNAL_GROUP_VALUE_AT_CHIP(port_a, sig_values), 8);
				ImGui::Separator();
				ui_signal_bits(8, "Port-B  In", 128, SIGNAL_GROUP_VALUE(port_b, sig_values), 8);
				ui_signal_bits(8, "Port-B Out", 128, SIGNAL_GROUP_VALUE_AT_CHIP(port_b, sig_values), 8);
			}

			if (ImGui::CollapsingHeader("Signals", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Columns(3);

				// header
				ImGui::NextColumn();
				ImGui::Text("Input"); ImGui::NextColumn();
				ImGui::Text("Output"); ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("/RES");					ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(RESET_B));		ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("CA1");						ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(CA1));			ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("CA2");						ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(CA2));			ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE_AT_CHIP(CA2));	ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("CB1");						ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(CB1));			ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("CB2");						ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(CB2));			ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE_AT_CHIP(CB2));	ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("/IRQA");					ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(IRQA_B));		ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE_AT_CHIP(IRQA_B));ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("/IRQB");					ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(IRQB_B));		ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE_AT_CHIP(IRQB_B));ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("CS0");						ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(CS0));			ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("CS1");						ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(CS1));			ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("/CS2");					ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(CS2_B));			ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("RS0");						ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(RS0));			ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("RS1");						ImGui::NextColumn();
				ui_signal(SIGNAL_VALUE(RS1));			ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::Separator();
			}
		}

		ImGui::End();
	}

private:
	ImVec2				position;
	const ImVec2		size = {240, 0};
	std::string			title;

	Chip6520 *			pia;
};


Panel::uptr_t panel_chip_6520_create(UIContext *ctx, struct ImVec2 pos, struct Chip6520 *pia) {
	return std::make_unique<PanelChip6520>(ctx, pos, pia);
}

