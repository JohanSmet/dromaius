// gui/panel_ieee488_tester.h - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_ieee488_tester.h"
#include "ui_context.h"

#include "perif_ieee488_tester.h"

#include "widgets.h"
#include "imgui_ex.h"

#define SIGNAL_PREFIX		CHIP_488TEST_
#define SIGNAL_OWNER		tester

class Panel488Tester : public Panel {
public:
	Panel488Tester(UIContext *ctx, ImVec2 pos, Perif488Tester *tester):
			Panel(ctx),
			position(pos),
			tester(tester) {
	}

	void display() override {
		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title, &stay_open)) {

			if (ImGui::CollapsingHeader("Control Signals", ImGuiTreeNodeFlags_DefaultOpen)) {

				ImGui::Columns(8);

				ImGui::NextColumn();
				ImGui::Text("/EOI");	ImGui::NextColumn();
				ImGui::Text("/DAV");	ImGui::NextColumn();
				ImGui::Text("/NRFD");	ImGui::NextColumn();
				ImGui::Text("/NDAC");	ImGui::NextColumn();
				ImGui::Text("/ATN");	ImGui::NextColumn();
				ImGui::Text("/SRQ");	ImGui::NextColumn();
				ImGui::Text("/IFC");	ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("Bus");	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(EOI_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(DAV_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(NRFD_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(NDAC_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(ATN_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(SRQ_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(IFC_B));	ImGui::NextColumn();

				ImGui::Text("Out");		ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(EOI_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(DAV_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(NRFD_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(NDAC_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(ATN_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(SRQ_B));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(IFC_B));	ImGui::NextColumn();

				ImGui::NextColumn();
				ImGui::Checkbox("##EOI", &tester->force_output_low[SIGNAL_ENUM(EOI_B)]);	ImGui::NextColumn();
				ImGui::Checkbox("##DAV", &tester->force_output_low[SIGNAL_ENUM(DAV_B)]);	ImGui::NextColumn();
				ImGui::Checkbox("##NRFD", &tester->force_output_low[SIGNAL_ENUM(NRFD_B)]);	ImGui::NextColumn();
				ImGui::Checkbox("##NDAC", &tester->force_output_low[SIGNAL_ENUM(NDAC_B)]);	ImGui::NextColumn();
				ImGui::Checkbox("##ATN", &tester->force_output_low[SIGNAL_ENUM(ATN_B)]);	ImGui::NextColumn();
				ImGui::Checkbox("##SRQ", &tester->force_output_low[SIGNAL_ENUM(SRQ_B)]);	ImGui::NextColumn();
				ImGui::Checkbox("##IFC", &tester->force_output_low[SIGNAL_ENUM(IFC_B)]);	ImGui::NextColumn();

				ImGui::Columns(1);
			}

			if (ImGui::CollapsingHeader("Data Signals", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Columns(9);
				ImGui::NextColumn();
				ImGui::Text("0");	ImGui::NextColumn();
				ImGui::Text("1");	ImGui::NextColumn();
				ImGui::Text("2");	ImGui::NextColumn();
				ImGui::Text("3");	ImGui::NextColumn();
				ImGui::Text("4");	ImGui::NextColumn();
				ImGui::Text("5");	ImGui::NextColumn();
				ImGui::Text("6");	ImGui::NextColumn();
				ImGui::Text("7");	ImGui::NextColumn();
				ImGui::Separator();

				ImGui::Text("Bus");	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(DIO0));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(DIO1));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(DIO2));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(DIO3));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(DIO4));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(DIO5));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(DIO6));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE(DIO7));	ImGui::NextColumn();

				ImGui::Text("Out");		ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(DIO0));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(DIO1));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(DIO2));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(DIO3));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(DIO4));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(DIO5));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(DIO6));	ImGui::NextColumn();
				ui_signal_short(SIGNAL_VALUE_AT_CHIP(DIO7));	ImGui::NextColumn();

				ImGui::NextColumn();
				ImGui::Checkbox("##DIO0", &tester->force_output_low[SIGNAL_ENUM(DIO0)]);	ImGui::NextColumn();
				ImGui::Checkbox("##DIO1", &tester->force_output_low[SIGNAL_ENUM(DIO1)]);	ImGui::NextColumn();
				ImGui::Checkbox("##DIO2", &tester->force_output_low[SIGNAL_ENUM(DIO2)]);	ImGui::NextColumn();
				ImGui::Checkbox("##DIO3", &tester->force_output_low[SIGNAL_ENUM(DIO3)]);	ImGui::NextColumn();
				ImGui::Checkbox("##DIO4", &tester->force_output_low[SIGNAL_ENUM(DIO4)]);	ImGui::NextColumn();
				ImGui::Checkbox("##DIO5", &tester->force_output_low[SIGNAL_ENUM(DIO5)]);	ImGui::NextColumn();
				ImGui::Checkbox("##DIO6", &tester->force_output_low[SIGNAL_ENUM(DIO6)]);	ImGui::NextColumn();
				ImGui::Checkbox("##DIO7", &tester->force_output_low[SIGNAL_ENUM(DIO7)]);	ImGui::NextColumn();

				ImGui::Columns(1);
			}

		}

		ImGui::End();
	}

private:
	ImVec2					position;
	const ImVec2			size = {330, 0};
	Perif488Tester *		tester;

	static constexpr const char *title = "IEEE-488 Tester";
};

///////////////////////////////////////////////////////////////////////////////
//
// interface
//

Panel::uptr_t panel_ieee488_tester_create(	class UIContext *ctx, struct ImVec2 pos,
										Perif488Tester *datassette) {
	return std::make_unique<Panel488Tester>(ctx, pos, datassette);
};


