// gui/popup_file_selector.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "popup_file_selector.h"
#include "imgui_ex.h"
#include "ui_context.h"

extern "C" {
#include <cute/cute_files.h>
}

PopupFileSelector::PopupFileSelector(UIContext *ctx, const char *path, const char *extension, const char *prefix) :
	title(ctx->unique_panel_id("Select file")),
	path(path),
	extension(extension),
	prefix(prefix) {
}

void PopupFileSelector::define_popup() {

	ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
	ImGui::SetNextWindowSize({200, 0});

    if (ImGui::BeginPopupModal(title.c_str())) {
		if (filenames.empty()) {
			ImGui::Text("No suitable files present!");
		} else {
			ImGui::Text("Click to select file");
			ImGui::Separator();

			for (const auto &entry : filenames) {
				if (ImGui::Selectable(entry.c_str())) {
					if (on_select_callback) {
						on_select_callback(entry);
					}
					break;
				}
			}
		}

		ImGui::Separator();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}

        ImGui::EndPopup();
    }
}

void PopupFileSelector::display_popup(on_select_callback_t on_select) {
	filenames = construct_directory_listing();
	on_select_callback = on_select;
	ImGui::OpenPopup(title.c_str());
}

std::vector<std::string> PopupFileSelector::construct_directory_listing() {
	cf_dir_t dir;
	std::vector<std::string> result;

	for (cf_dir_open(&dir, path.c_str()); dir.has_next != 0; cf_dir_next(&dir)) {
		cf_file_t file;
		cf_read_file(&dir, &file);

		// skip anything that isn't a regular file with the wanted extension
		if (file.is_reg == 0 || cf_match_ext(&file, extension.c_str()) == 0) {
			continue;
		}

		// optionally check prefix
		if (!prefix.empty() && prefix.compare(0, prefix.size(), file.name, prefix.size())) {
			continue;
		}

		result.push_back(file.name);
	}

	cf_dir_close(&dir);

	std::sort(begin(result), end(result));

	return result;
}

