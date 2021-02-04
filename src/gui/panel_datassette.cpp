// gui/panel_datassette.h - Johan Smet - BSD-3-Clause (see LICENSE)

#define SIGNAL_ARRAY_STYLE
#include "panel_datassette.h"
#include "ui_context.h"

#include "perif_datassette_1530.h"

#include "widgets.h"
#include "imgui_ex.h"
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "popup_file_selector.h"

#define SIGNAL_PREFIX		PIN_DS1530_
#define SIGNAL_OWNER		datassette

namespace {

static std::string tap_path = "runtime/commodore_pet/tap";

static inline std::string path_for_tap_file(const std::string & filename) {
	std::string result = tap_path;
	result += "/";
	result += filename;

	return result;
}

} // unnamed namespace

class PanelDatassette : public Panel {
public:
	PanelDatassette(UIContext *ctx, ImVec2 pos, PerifDatassette *datassette):
			Panel(ctx),
			position(pos),
			datassette(datassette) {
	}

	void display() override {
		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		bool load_tap = false;

		if (ImGui::Begin(title, &stay_open)) {

			if (ImGui::Button("Record")) {
				perif_datassette_key_pressed(datassette, DS_KEY_RECORD);
			}
			ImGui::SameLine();

			if (ImGui::Button("Play")) {
				perif_datassette_key_pressed(datassette, DS_KEY_PLAY);
			}
			ImGui::SameLine();

			if (ImGui::Button("Rewind")) {
				perif_datassette_key_pressed(datassette, DS_KEY_REWIND);
			}
			ImGui::SameLine();

			if (ImGui::Button("F.Fwd")) {
				perif_datassette_key_pressed(datassette, DS_KEY_FFWD);
			}
			ImGui::SameLine();

			if (ImGui::Button("Stop")) {
				perif_datassette_key_pressed(datassette, DS_KEY_STOP);
			}
			ImGui::SameLine();

			if (ImGui::Button("Eject")) {
				perif_datassette_key_pressed(datassette, DS_KEY_EJECT);
			}

			load_tap = ImGui::Button("Load");
			ImGui::SameLine();

			if (ImGui::Button("New")) {
				perif_datassette_new_tap(datassette, path_for_tap_file(tap_filename).c_str());
			}
			ImGui::SameLine();
			ImGui::InputText("##new_tap", &tap_filename);

			auto origin = ImGui::GetCursorPos();
			ui_signal(20, "/SENSE", SIGNAL_READ_NEXT(SENSE), ACTLO_ASSERT);
			ui_signal(20, "MOTOR", SIGNAL_READ_NEXT(MOTOR), ACTHI_ASSERT);
			ui_signal(20, "READ", SIGNAL_READ_NEXT(DATA_FROM_DS), ACTHI_ASSERT);
			ui_signal(20, "WRITE", SIGNAL_READ_NEXT(DATA_TO_DS), ACTHI_ASSERT);

			ImGui::SetCursorPos({140, origin.y});
			ImGui::Text("Position %.3lld (%s)", PS_TO_S(datassette->offset_ps), ds_state_strings[datassette->state]);
		}

		tap_selection->define_popup();
		if (load_tap) {
			tap_selection->display_popup([&](std::string selected_file) {
				perif_datassette_load_tap_from_file(datassette, path_for_tap_file(selected_file).c_str());
			});
		}

		ImGui::End();
	}

private:
	ImVec2					position;
	const ImVec2			size = {330, 0};
	PerifDatassette *		datassette;

	std::string				tap_filename = "new.tap";

	PopupFileSelector::uptr_t tap_selection = PopupFileSelector::make_unique(ui_context, tap_path.c_str() , ".tap", "");

	static constexpr const char *title = "Datasette - 1530";
	static constexpr const char *ds_state_strings[] = {"Idle", "Tape Loaded", "Playing", "Recording", "Rewinding", "Fast-Forwarding"};
};

///////////////////////////////////////////////////////////////////////////////
//
// interface
//

Panel::uptr_t panel_datassette_create(	class UIContext *ctx, struct ImVec2 pos,
										PerifDatassette *datassette) {
	return std::make_unique<PanelDatassette>(ctx, pos, datassette);
};


