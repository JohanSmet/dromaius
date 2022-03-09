// gui/panel_chip_6522.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a 6522 Peripheral Interace Adapter

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
		SignalValue sig_values[8];

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {

			if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen)) {

				ui_register_8bit_binary(8, "Interrupt Flags", via->reg_ifr);
				ui_register_8bit_binary(8, "Interrupt Enable", via->reg_ier);

				ui_register_8bit_binary(8, "Periph. Control", via->reg_pcr);
				ui_register_8bit_binary(8, "Auxiliary Control", via->reg_acr);

				ui_register_8bit(8,  "T1 - low latch", via->reg_t1l_l);
				ui_register_8bit(8,  "T1 - high latch", via->reg_t1l_h);
				ui_register_16bit(8, "T1 - counter", via->reg_t1c);

				ui_register_8bit(8,  "T2 - latch", via->reg_t2l_l);
				ui_register_16bit(8, "T2 - counter", via->reg_t2c);

				ui_register_8bit(8, "Shift Register", via->reg_sr);

				ui_register_8bit(8, "Input Latch A", via->reg_ila);
				ui_register_8bit(8, "Output reg A", via->reg_ora);
				ui_register_8bit_binary(8, "Data Direction A", via->reg_ddra);

				ui_register_8bit(8, "Input Latch B", via->reg_ilb);
				ui_register_8bit(8, "Output reg B", via->reg_orb);
				ui_register_8bit_binary(8, "Data Direction B", via->reg_ddrb);
			}

			if (ImGui::CollapsingHeader("Ports", ImGuiTreeNodeFlags_DefaultOpen)) {

				if (ui_bit_array_table_start("databus", 8, true, "Data  ")) {

					SIGNAL_GROUP_VALUE(data, sig_values);
					ui_bit_array_table_row("Bus", 8, sig_values);

					SIGNAL_GROUP_VALUE_AT_CHIP(data, sig_values);
					ui_bit_array_table_row("Out", 8, sig_values);

					ImGui::EndTable();
				}

				if (ui_bit_array_table_start("port_a", 8, true, "Port-A")) {

					SIGNAL_GROUP_VALUE(port_a, sig_values);
					ui_bit_array_table_row("Bus", 8, sig_values);

					SIGNAL_GROUP_VALUE_AT_CHIP(port_a, sig_values);
					ui_bit_array_table_row("Out", 8, sig_values);

					ImGui::EndTable();
				}

				if (ui_bit_array_table_start("port_b", 8, true, "Port-B")) {

					SIGNAL_GROUP_VALUE(port_b, sig_values);
					ui_bit_array_table_row("Bus", 8, sig_values);

					SIGNAL_GROUP_VALUE_AT_CHIP(port_b, sig_values);
					ui_bit_array_table_row("Out", 8, sig_values);

					ImGui::EndTable();
				}
			}

			if (ImGui::CollapsingHeader("Signals", ImGuiTreeNodeFlags_DefaultOpen)) {
				if (ImGui::BeginTable("signals_a", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit)) {
					ImGui::TableSetupColumn("##type");
					ImGui::TableSetupColumn("CS1");
					ImGui::TableSetupColumn("/CS2");
					ImGui::TableSetupColumn("RS0");
					ImGui::TableSetupColumn("RS1");
					ImGui::TableSetupColumn("RS2");
					ImGui::TableSetupColumn("RS3");
					ImGui::TableHeadersRow();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();	ImGui::Text("Bus");
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(CS1));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(CS2_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(RS0));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(RS1));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(RS2));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(RS3));

					ImGui::EndTable();
				}

				if (ImGui::BeginTable("signals_b", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit)) {
					ImGui::TableSetupColumn("##type");
					ImGui::TableSetupColumn("CA1");
					ImGui::TableSetupColumn("CA2");
					ImGui::TableSetupColumn("CB1");
					ImGui::TableSetupColumn("CB2");
					ImGui::TableSetupColumn("/IRQ");
					ImGui::TableSetupColumn("/RES");
					ImGui::TableHeadersRow();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();	ImGui::Text("Bus");
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(CA1));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(CA2));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(CB1));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(CB2));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(IRQ_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(RESET_B));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();	ImGui::Text("Out");
					ImGui::TableNextColumn();
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(CA2));
					ImGui::TableNextColumn();
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(CB2));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(IRQ_B));
					ImGui::TableNextColumn();

					ImGui::EndTable();
				}
			}
		}

		ImGui::End();
	}

private:
	ImVec2				position;
	std::string			title;

	Chip6522 *			via;

};


Panel::uptr_t panel_chip_6522_create(UIContext *ctx, struct ImVec2 pos, struct Chip6522 *via) {
	return std::make_unique<PanelChip6522>(ctx, pos, via);
}

