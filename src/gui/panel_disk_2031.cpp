// gui/panel_disk_2031.h - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_disk_2031.h"
#include "ui_context.h"

#include "perif_disk_2031.h"

#include "widgets.h"
#include "imgui_ex.h"
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

			load_d64 = ImGui::Button("Load");

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

				ImGui::Columns(1);
			}

			if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SliderInt("Address", (int *) &tester->address, 0, 15);
			}

			if (ImGui::CollapsingHeader("State", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Text("State: %s", STATE_LABELS[tester->bus_state]);
				ImGui::Text("Comm. State %s", COMM_STATE_LABELS[tester->comm_state]);
				ImGui::Text("Active channel = %ld", tester->active_channel);
			}

			if (ImGui::CollapsingHeader("Channels", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Columns(3);

				for (int i = 0; i < FD2031_MAX_CHANNELS; ++i) {
					ImGui::Text("%d", i);													ImGui::NextColumn();
					ImGui::Text("%s", (tester->channels[i].open) ? "Open" : "Closed");		ImGui::NextColumn();
					ImGui::Text("%s", tester->channels[i].name);							ImGui::NextColumn();
				}

				ImGui::Columns(1);
			}
		}

		d64_selection->define_popup();
		if (load_d64) {
			d64_selection->display_popup([&](std::string selected_file) {
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
	PerifDisk2031 *		tester;
	std::string				d64_filename = "new.d64";

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


