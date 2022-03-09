// gui/panel_cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a MOS 6502

#include "panel_cpu_6502.h"

#include "widgets.h"
#include "cpu_6502.h"
#include "ui_context.h"
#include <imgui.h>

#define SIGNAL_OWNER		cpu
#define SIGNAL_PREFIX		PIN_6502_

class PanelCpu6502 : public Panel {
public:
	PanelCpu6502(UIContext *ctx, ImVec2 pos, Cpu6502 *cpu) :
		Panel(ctx),
		position(pos),
		cpu(cpu) {
		title = ui_context->unique_panel_id("CPU - MOS 6502");
	}

	void display() override {
		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize({0, 0}, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {

			if (ImGui::CollapsingHeader(txt_header_registers, ImGuiTreeNodeFlags_DefaultOpen)) {
				ui_register_8bit(8, "Accumulator", cpu->reg_a);
				ui_register_8bit(8, "index-X", cpu->reg_x);
				ui_register_8bit(8, "index-Y", cpu->reg_y);
				ui_register_8bit(8, "Stack Pointer", cpu->reg_sp);
				ui_register_16bit(8, "Program Counter", cpu->reg_pc);
				ui_register_8bit(8, "Instruction", cpu->reg_ir);
			}

			if (ImGui::CollapsingHeader(txt_header_flags, ImGuiTreeNodeFlags_DefaultOpen)) {

				if (ImGui::BeginTable("flags", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit)) {

					// header
					ImGui::TableSetupColumn(ImGuiEx::string_format("$%.2x", cpu->reg_p).c_str());
					ImGui::TableSetupColumn("N");
					ImGui::TableSetupColumn("V");
					ImGui::TableSetupColumn("-");
					ImGui::TableSetupColumn("B");
					ImGui::TableSetupColumn("D");
					ImGui::TableSetupColumn("I");
					ImGui::TableSetupColumn("Z");
					ImGui::TableSetupColumn("C");
					ImGui::TableHeadersRow();

					// values
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TableNextColumn();	ImGui::Text("%d", FLAG_IS_SET(cpu->reg_p, FLAG_6502_N));
					ImGui::TableNextColumn();	ImGui::Text("%d", FLAG_IS_SET(cpu->reg_p, FLAG_6502_V));
					ImGui::TableNextColumn();	ImGui::Text("%d", FLAG_IS_SET(cpu->reg_p, FLAG_6502_E));
					ImGui::TableNextColumn();	ImGui::Text("%d", FLAG_IS_SET(cpu->reg_p, FLAG_6502_B));
					ImGui::TableNextColumn();	ImGui::Text("%d", FLAG_IS_SET(cpu->reg_p, FLAG_6502_D));
					ImGui::TableNextColumn();	ImGui::Text("%d", FLAG_IS_SET(cpu->reg_p, FLAG_6502_I));
					ImGui::TableNextColumn();	ImGui::Text("%d", FLAG_IS_SET(cpu->reg_p, FLAG_6502_Z));
					ImGui::TableNextColumn();	ImGui::Text("%d", FLAG_IS_SET(cpu->reg_p, FLAG_6502_C));

					ImGui::EndTable();
				}
			}

			if (ImGui::CollapsingHeader(txt_header_signals, ImGuiTreeNodeFlags_DefaultOpen)) {

				// address bus
				ImGui::Text("Address Bus: $%.4x", SIGNAL_GROUP_READ_U16(address));
				if (ui_bit_array_table_start("address_bits", 16, false)) {
					SignalValue sig_addr[16];

					SIGNAL_GROUP_VALUE(address, sig_addr);
					ui_bit_array_table_row(nullptr, 16, sig_addr);

					ui_bit_array_table_end();
				}

				// data bus
				ImGui::Text("Data Bus: $%.2x", SIGNAL_GROUP_READ_U8(data));

				if (ui_bit_array_table_start("data_bits", 8, true)) {

					SignalValue sig_data[8];

					SIGNAL_GROUP_VALUE(data, sig_data);
					ui_bit_array_table_row("Bus", 8, sig_data);

					SIGNAL_GROUP_VALUE_AT_CHIP(data, sig_data);
					ui_bit_array_table_row("Out", 8, sig_data);

					ImGui::EndTable();
				}

				// remaining signals
				ImGui::Text("Remaining Signals");

				if (ImGui::BeginTable("signals", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit)) {
					ImGui::TableSetupColumn("##type");
					ImGui::TableSetupColumn("/RES");
					ImGui::TableSetupColumn("/IRQ");
					ImGui::TableSetupColumn("/NMI");
					ImGui::TableSetupColumn("RDY");
					ImGui::TableSetupColumn("SYNC");
					ImGui::TableSetupColumn("R/W");
					ImGui::TableHeadersRow();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();	ImGui::Text("Bus");
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(RES_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(IRQ_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(NMI_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(RDY));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(SYNC));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(RW));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();	ImGui::Text("Out");
					ImGui::TableNextColumn();
					ImGui::TableNextColumn();
					ImGui::TableNextColumn();
					ImGui::TableNextColumn();
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(SYNC));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(RW));

					ImGui::EndTable();
				}
			}
		}

		ImGui::End();
	}

private:
	static constexpr const char *txt_header_registers = "Registers";
	static constexpr const char *txt_header_flags = "Status Flags";
	static constexpr const char *txt_header_signals = "Signals";

	static constexpr const char *table_header_digits[] = {
		"0", "1", "2", "3", "4", "5", "6", "7",
		"8", "9", "A", "B", "C", "D", "E", "F"
	};


private:
	ImVec2				position;
	std::string			title;

	Cpu6502	*			cpu;
};

Panel::uptr_t panel_cpu_6502_create(UIContext *ctx, struct ImVec2 pos, struct Cpu6502 *cpu) {
	return std::make_unique<PanelCpu6502>(ctx, pos, cpu);
}

