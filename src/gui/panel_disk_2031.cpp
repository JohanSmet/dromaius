// gui/panel_disk_2031.h - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_disk_2031.h"
#include "ui_context.h"

#include "perif_disk_2031.h"

#include "widgets.h"
#include "imgui_ex.h"
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include "popup_file_selector.h"

#define SIGNAL_PREFIX		PERIF_FD2031_
#define SIGNAL_OWNER		tester

namespace {

static std::string d64_path = "runtime/commodore_pet/d64";

static inline std::string path_for_d64_file(const std::string & filename) {
	std::string result = d64_path;
	result += "/";
	result += filename;

	return result;
}

}

class PanelDisk2031 : public Panel {
public:
	PanelDisk2031(UIContext *ctx, ImVec2 pos, PerifDisk2031 *tester):
			Panel(ctx),
			position(pos),
			tester(tester) {
	}

	void display() override {
		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		bool load_d64 = false;

		if (ImGui::Begin(title, &stay_open)) {

			load_d64 = ImGui::Button("Load Floppy File");

			ImGui::SameLine(0, 15);
			ImGui::Text("(%s)", loaded_filename.c_str());
			ImGui::Spacing();

			if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SliderInt("Address", (int *) &tester->address, 0, 15);
			}

			if (ImGui::CollapsingHeader("Control Signals", ImGuiTreeNodeFlags_DefaultOpen)) {

				if (ImGui::BeginTable("control_signals", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit)) {
					ImGui::TableSetupColumn("##type");
					ImGui::TableSetupColumn("/EOI");
					ImGui::TableSetupColumn("/DAV");
					ImGui::TableSetupColumn("/NRFD");
					ImGui::TableSetupColumn("/NDAC");
					ImGui::TableSetupColumn("/ATN");
					ImGui::TableSetupColumn("/SRQ");
					ImGui::TableSetupColumn("/IFC");
					ImGui::TableHeadersRow();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();	ImGui::Text("Bus");
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(EOI_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(DAV_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(NRFD_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(NDAC_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(ATN_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(SRQ_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE(IFC_B));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();	ImGui::Text("Out");
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(EOI_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(DAV_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(NRFD_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(NDAC_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(ATN_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(SRQ_B));
					ImGui::TableNextColumn();	ui_signal_short(SIGNAL_VALUE_AT_CHIP(IFC_B));

					ImGui::EndTable();
				}
			}

			if (ImGui::CollapsingHeader("Data Signals", ImGuiTreeNodeFlags_DefaultOpen)) {
				SignalValue sig_values[8];

				if (ui_bit_array_table_start("databus", 8, true, "I/0")) {

					SIGNAL_GROUP_VALUE(dio, sig_values);
					ui_bit_array_table_row("Bus", 8, sig_values);

					SIGNAL_GROUP_VALUE_AT_CHIP(dio, sig_values);
					ui_bit_array_table_row("Out", 8, sig_values);

					ImGui::EndTable();
				}
			}

			if (ImGui::CollapsingHeader("State")) {
				ImGui::Text("State: %s", STATE_LABELS[tester->bus_state]);
				ImGui::Text("Comm. State %s", COMM_STATE_LABELS[tester->comm_state]);
				ImGui::Text("Active channel = %ld", tester->active_channel);
			}

			if (ImGui::CollapsingHeader("Channels")) {
				if (ImGui::BeginTable("channels", 3, ImGuiTableFlags_Borders)) {
					ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 100);
					ImGui::TableSetupColumn("Filename", ImGuiTableColumnFlags_WidthStretch);

					for (int i = 0; i < FD2031_MAX_CHANNELS; ++i) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();	ImGui::Text("%d", i);
						ImGui::TableNextColumn();	ImGui::Text("%s", (tester->channels[i].open) ? "Open" : "Closed");
						ImGui::TableNextColumn();	ImGui::Text("%s", tester->channels[i].name);
					}

					ImGui::EndTable();
				}
			}
		}

		d64_selection->define_popup();
		if (load_d64) {
			d64_selection->display_popup([&](std::string selected_file) {
				loaded_filename = selected_file;
				perif_fd2031_load_d64_from_file(tester, path_for_d64_file(selected_file).c_str());
			});
		}

		ImGui::End();
	}

private:
	constexpr static const char *STATE_LABELS[] = {
		"Idle",
		"AcceptorStart",
		"AcceptorReady",
		"AcceptorDataAvailable",
		"AcceptorDataTaken",
		"SourceStart",
		"SourceReady",
		"SourceDataValid",
		"SourceDataTaken"
	};

	constexpr static const char *COMM_STATE_LABELS[] = {
		"Idle",
		"Listening",
		"StartTalking",
		"Talking",
		"Opening"
	};

private:
	ImVec2					position;
	const ImVec2			size = {330, 0};
	PerifDisk2031 *			tester;

	std::string				loaded_filename = "no .d64 loaded";

	PopupFileSelector::uptr_t d64_selection = PopupFileSelector::make_unique(ui_context, d64_path.c_str() , ".d64", "");

	static constexpr const char *title = "Floppy Disk (Commodore 2031)";
};

///////////////////////////////////////////////////////////////////////////////
//
// interface
//

Panel::uptr_t panel_fd2031_create(	class UIContext *ctx, struct ImVec2 pos,
										PerifDisk2031 *tester) {
	return std::make_unique<PanelDisk2031>(ctx, pos, tester);
};


