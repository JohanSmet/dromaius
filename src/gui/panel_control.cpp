// gui/panel_control.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI panel to control the emulator

#include "panel_control.h"
#include "ui_context.h"
#include "context.h"
#include "utils.h"
#include "dev_minimal_6502.h"

#include <imgui.h>
#include <stb/stb_ds.h>
#include <string>

namespace {

static inline std::string append_path(const char *path, const char *filename) {
	std::string result = path;
	result += "/";
	result += filename;

	return result;
}

} // unnamed namespace

///////////////////////////////////////////////////////////////////////////////
//
// private types
//


class PanelControl : public Panel {
public:
	PanelControl(UIContext *ctx, ImVec2 pos, const char *data_path) :
		Panel(ctx),
		position(pos),
		data_path(data_path) {

		dir_list_files(data_path, ".bin", "rom_", &rom_files);
		dir_list_files(data_path, ".bin", "ram_", &ram_files);

		if (arrlen(rom_files) > 0) {
			current_rom = 0;
			load_current_rom();
		}
	}

	void display() override {
		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		ImGui::Begin(title);

			if (ImGui::Button("Single")) {
				dms_single_step(ui_context->dms_ctx);
			}
			ImGui::SameLine();

			if (ImGui::Button("Step")) {
				dms_single_instruction(ui_context->dms_ctx);
			}
			ImGui::SameLine();

			if (ImGui::Button("Run")) {
				dms_run(ui_context->dms_ctx);
			}
			ImGui::SameLine();

			if (ImGui::Button("Pause")) {
				dms_pause(ui_context->dms_ctx);
			}
			ImGui::SameLine();

			if (ImGui::Button("Reset")) {
				dms_reset(ui_context->dms_ctx);
			}

			if (ImGui::Combo("ROM-Image", &current_rom, rom_files, arrlen(rom_files))) {
				load_current_rom();
			}

			if (ImGui::Combo("RAM-Image", &current_ram, rom_files, arrlen(ram_files))) {
				load_current_ram();
			}


		ImGui::End();
	}

private:
	void load_current_rom() {
		if (current_rom < 0) {
			return;
		}

		auto full_path = append_path(data_path, rom_files[current_rom]);
		dev_minimal_6502_rom_from_file(dms_get_device(ui_context->dms_ctx), full_path.c_str());
	}

	void load_current_ram() {
		if (current_ram < 0) {
			return;
		}

		auto full_path = append_path(data_path, rom_files[current_ram]);
		dev_minimal_6502_ram_from_file(dms_get_device(ui_context->dms_ctx), full_path.c_str());
	}

private:
	ImVec2					position;
	const ImVec2			size = {360, 0};

	const char *			data_path;
	const char **			rom_files = nullptr;
	const char **			ram_files = nullptr;
	int						current_rom = -1;
	int						current_ram = -1;

	static constexpr const char *title = "Emulator Control";
};

Panel::uptr_t panel_control_create(UIContext *ctx, ImVec2 pos, const char *data_path) {
	auto panel = std::make_unique<PanelControl>(ctx, pos, data_path);
	return std::move(panel);
}
