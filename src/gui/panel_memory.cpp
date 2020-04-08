// gui/panel_memory.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI-panel to visualize a block of memory

#include "panel_memory.h"

#include <cassert>
#include <imgui.h>
#include <stb/stb_ds.h>

#include "filt_6502_asm.h"
#include "ui_context.h"

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

class PanelMemory : public Panel {
public:
	PanelMemory(UIContext *ctx, ImVec2 pos, const char *title,
				const uint8_t *data, size_t data_size, size_t data_offset) :
		Panel(ctx),
		position(pos),
		title(title),
		mem(data),
		mem_size(data_size),
		mem_offset(data_offset) {
	}

	void display() override {

		static const char *display_types[] = {"Raw", "6502 Disassembly", "PET Screen"};

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {

			ImGui::Combo("Display type", &display_type, display_types, sizeof(display_types) / sizeof(const char *));

			switch (display_type) {
				case DT_RAW:
					memory_display_raw();
					break;
				case DT_DISASM:
					memory_display_disasm_6502();
					break;
				case DT_PETSCREEN:
					memory_display_petscreen();
					break;
			}
		}
		ImGui::End();
	}

	void memory_display_raw() {

		ImGui::BeginChild("raw", ImGui::GetContentRegionAvail(), false, 0);

		for (auto index = 0u; index < mem_size; ) {

			ImGui::Text("%.4lx:", mem_offset + index);

			for (int block = 0; block < 4; ++block) {

				if (block > 0) {
					ImGui::SameLine(0, 6);
					ImGui::TextUnformatted("");
				}

				for (int i = 0; i < 4 && index < mem_size; ++i) {
					ImGui::SameLine();
					ImGui::Text("%.2x", mem[index++]);
				}
			}
		}

		ImGui::EndChild();
	}

	void memory_display_disasm_6502() {

		constexpr const char *symbols[] = {" ", ">"};
		static const ImVec4 colors[] = { ImGui::GetStyle().Colors[ImGuiCol_Text], ImVec4(1.0f, 1.0f, 0.0f, 1.0f)};

		auto region = ImGui::GetContentRegionAvail();
		region.y -= 20;
		ImGui::BeginChild("raw", region, false, 0);

		char *line = NULL;

		for (size_t index = 0; index < mem_size; ) {
			bool is_current = (index + mem_offset) == (size_t) ui_context->last_pc;

			if (is_current && follow_pc) {
				ImGui::SetScrollHereY();
			}

			index += filt_6502_asm_line(mem, mem_size, index, mem_offset, &line);

			ImGui::TextColored(colors[is_current], "%s%s", symbols[is_current], line);
			arrsetlen(line, 0);
		}

		ImGui::EndChild();

		ImGui::Checkbox("Follow PC", &follow_pc);
	}

	void memory_display_petscreen() {
		ImGui::BeginChild("pet", ImGui::GetContentRegionAvail(), false, 0);

		ImGui::PushFont(pet_font);

		char buffer[4] = {0};

		for (auto index = 0u; index < mem_size; ++index) {
			if (index % 40 > 0) {
				ImGui::SameLine();
			}

			int c = 0xE000 + mem[index];
			buffer[0] = (char)(0xe0 + (c >> 12));
			buffer[1] = (char)(0x80 + ((c>> 6) & 0x3f));
			buffer[2] = (char)(0x80 + ((c ) & 0x3f));

			ImGui::Text("%s", buffer);
		}

		ImGui::PopFont();

		ImGui::EndChild();
	}

	static void load_pet_font() {
		if (pet_font != nullptr) {
			return;
		}

		ImVector<ImWchar>	ranges;
		ImFontGlyphRangesBuilder builder;

		for (ImWchar i = 0; i < 256; ++i) {
			builder.AddChar(static_cast<ImWchar>((0xE000 + i) & 0xffff));
		}
		builder.BuildRanges(&ranges);

		auto io = ImGui::GetIO();
		pet_font = io.Fonts->AddFontFromFileTTF("runtime/commodore_pet/font/PetMe.ttf", 14.0f, nullptr, ranges.Data);
		assert(pet_font);
		io.Fonts->Build();
	}

private:
	enum DISPLAY_TYPES {
		DT_RAW = 0,
		DT_DISASM = 1,
		DT_PETSCREEN = 2
	};

private:
	ImVec2					position;
	const ImVec2			size = {440, 220};
	std::string				title;

	const uint8_t *			mem;
	size_t					mem_size;
	size_t					mem_offset;

	int						display_type = 0;
	bool					follow_pc = false;

	static ImFont *			pet_font;
};

ImFont * PanelMemory::pet_font = nullptr;

Panel::uptr_t panel_memory_create(UIContext *ctx, struct ImVec2 pos, const char *title,
								  const uint8_t *data, size_t data_size, size_t data_offset) {

	return std::make_unique<PanelMemory>(ctx, pos, title, data, data_size, data_offset);
}

void panel_memory_load_fonts() {
	PanelMemory::load_pet_font();
}
