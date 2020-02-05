// gui/panel_cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to display information about a MOS 6502

#include "panel_cpu_6502.h"

#include "widgets.h"
#include "cpu_6502.h"
#include "ui_context.h"

#define SIGNAL_POOL			cpu->signal_pool
#define SIGNAL_COLLECTION	cpu->signals

class PanelCpu6502 : public Panel {
public:
	PanelCpu6502(UIContext *ctx, ImVec2 pos, Cpu6502 *cpu) :
		Panel(ctx),
		position(pos),
		cpu(cpu) {
		title = ui_context->unique_panel_id("CPU -  MOS 6502");
	}

	void display() override {
		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
		
		if (ImGui::Begin(title.c_str(), &stay_open)) {
			auto origin = ImGui::GetCursorPos();

			// left column
			ui_register_8bit(8, "Accumulator", cpu->reg_a);
			ui_register_8bit(8, "index-X", cpu->reg_x);
			ui_register_8bit(8, "index-Y", cpu->reg_y);
			ui_register_8bit(8, "Stack Pointer", cpu->reg_sp);
			ui_register_16bit(8, "Program Counter", cpu->reg_pc);
			ui_register_8bit(8, "Instruction", cpu->reg_ir);
			ui_register_8bit(8, "Processor Status", cpu->reg_p);
			ui_register_16bit(8, "Address Bus", SIGNAL_NEXT_UINT16(bus_address));
			ui_register_8bit(8, "Data Bus", SIGNAL_NEXT_UINT8(bus_data));

			// right column
			ImGui::SetCursorPos({origin.x + 180, origin.y});

			ImGuiEx::Text(10, "N", ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, "V", ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, "-", ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, "B", ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, "D", ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, "I", ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, "Z", ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, "C", ImGuiEx::TAH_CENTER);
			ImGui::NewLine();

			ImGui::SetCursorPos({origin.x + 180, ImGui::GetCursorPosY()});
			ImGuiEx::Text(10, ImGuiEx::string_format("%d", cpu->p_negative_result), ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, ImGuiEx::string_format("%d", cpu->p_overflow), ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, ImGuiEx::string_format("%d", cpu->p_expension), ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, ImGuiEx::string_format("%d", cpu->p_break_command), ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, ImGuiEx::string_format("%d", cpu->p_decimal_mode), ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, ImGuiEx::string_format("%d", cpu->p_interrupt_disable), ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, ImGuiEx::string_format("%d", cpu->p_zero_result), ImGuiEx::TAH_CENTER);
			ImGuiEx::Text(10, ImGuiEx::string_format("%d", cpu->p_carry), ImGuiEx::TAH_CENTER);
			ImGui::NewLine();

			ui_signal(180, "/RES", SIGNAL_NEXT_BOOL(reset_b), ACTLO_ASSERT);
			ui_signal(180, "/IRQ", SIGNAL_NEXT_BOOL(irq_b), ACTLO_ASSERT);
			ui_signal(180, "/NMI", SIGNAL_NEXT_BOOL(nmi_b), ACTLO_ASSERT);
			ui_signal(180, "RDY",  SIGNAL_NEXT_BOOL(rdy), ACTHI_ASSERT);
			ui_signal(180, "SYNC", SIGNAL_NEXT_BOOL(sync), ACTHI_ASSERT);
			ui_signal(180, "R/W",  SIGNAL_NEXT_BOOL(rw), ACTHI_ASSERT);
		}

		ImGui::End();
	}


private:
	ImVec2				position;
	const ImVec2		size = {410, 0};
	std::string			title;

	Cpu6502	*			cpu;
};

Panel::uptr_t panel_cpu_6502_create(UIContext *ctx, struct ImVec2 pos, struct Cpu6502 *cpu) {
	return std::make_unique<PanelCpu6502>(ctx, pos, cpu);
}

