// gui/panel_memory.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// UI-panel to visualize a block of memory
//
// FIXME: optimize by only copying/processing the memory actually displayed by the window

#include "panel_memory.h"

#include <cassert>
#include <stb/stb_ds.h>
#include <utils.h>

#include "filt_6502_asm.h"
#include "ui_context.h"
#include "device.h"
#include "imgui_ex.h"

///////////////////////////////////////////////////////////////////////////////
//
// private types
//

class PanelMemory : public Panel {
public:
	PanelMemory(UIContext *ctx, ImVec2 pos, const char *title,
				size_t data_size, size_t data_offset) :
		Panel(ctx),
		position(pos),
		title(title),
		mem_size(data_size),
		mem_offset(data_offset) {
	}

	void init() override {
		footer_height = ImGuiEx::LabelHeight("B") + ImGui::GetStyle().FramePadding.y * 2.0f;
	}

	void display() override {

		static const char *display_types[] = {"Raw", "6502 Disassembly", "PET Screen"};

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title.c_str(), &stay_open)) {

			ImGui::AlignTextToFramePadding();

			ImGui::Text("Display Type");
			ImGui::SameLine();

			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::Combo("##display_type", &display_type, display_types, sizeof(display_types) / sizeof(const char *));

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

		auto region = ImGui::GetContentRegionAvail();
		region.y -= footer_height;
		ImGui::BeginChild("raw", region, false, 0);

		for (auto index = 0u; index < mem_size; ) {

			if (jump_addr >= 0 && (size_t) jump_addr >= (index + mem_offset) && (size_t) jump_addr < (index + mem_offset + 16)) {
				ImGui::SetScrollHereY();
				jump_addr = -1;
			}

			// copy a line from the device memory
			uint8_t buffer[16];
			ui_context->device->read_memory(ui_context->device, mem_offset + index, 16, buffer);

			ImGui::Text("%.4lx:", mem_offset + index);

			for (int i = 0; i < 16; ++i) {
				if (i > 0 && (i % 4) == 0) {
					ImGui::SameLine(0, 6);
					ImGui::TextUnformatted("");
				}

				ImGui::SameLine();
				ImGui::Text("%.2x", buffer[i]);
			}

			index += 16;
		}

		ImGui::EndChild();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Jump to Address");
		ImGui::SameLine();

		ImGui::SetNextItemWidth(64);
		if (ImGui::InputText("##addr", input_addr, 5,
							 ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
			string_to_hexint(input_addr, &jump_addr);
		}
	}

	void memory_display_disasm_6502() {

		constexpr const char *symbols[] = {" ", ">"};
		static const ImVec4 colors[] = { ImGui::GetStyle().Colors[ImGuiCol_Text], ImVec4(1.0f, 1.0f, 0.0f, 1.0f)};

		auto region = ImGui::GetContentRegionAvail();
		region.y -= footer_height;
		ImGui::BeginChild("raw", region, false, 0);

		char *line = NULL;

		for (size_t index = 0; index < mem_size; ) {
			bool is_current = (index + mem_offset) == (size_t) ui_context->last_pc;

			if (is_current && follow_pc) {
				ImGui::SetScrollHereY();
			} else if (jump_addr >= 0 && (size_t) jump_addr == (index + mem_offset)) {
				ImGui::SetScrollHereY();
				jump_addr = -1;
			}

			// copy a few bytes from the device memory
			uint8_t buffer[16];
			ui_context->device->read_memory(ui_context->device, mem_offset + index, 16, buffer);

			index += filt_6502_asm_line(buffer, 16, 0, mem_offset + index, &line);

			ImGui::TextColored(colors[is_current], "%s%s", symbols[is_current], line);
			arrsetlen(line, 0);
		}

		ImGui::EndChild();

		ImGui::AlignTextToFramePadding();
		ImGui::Checkbox("Follow PC", &follow_pc);
		ImGui::SameLine(0, 15);

		ImGui::Text("Jump to Address");
		ImGui::SameLine();

		ImGui::SetNextItemWidth(64);
		if (ImGui::InputText("##addr", input_addr, 5,
							 ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
			string_to_hexint(input_addr, &jump_addr);
		}
	}

	void memory_display_petscreen() {
		ImGui::BeginChild("pet", ImGui::GetContentRegionAvail(), false, 0);

		ImGui::PushFont(pet_font);

		char buffer[4] = {0};

		// copy the screen from the device
		uint8_t screen[1000];
		ui_context->device->read_memory(ui_context->device, mem_offset, 1000, screen);

		for (auto index = 0u; index < 1000; ++index) {
			if (index % 40 > 0) {
				ImGui::SameLine();
			}

			int c = 0xE000 + screen[index];
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
		pet_font = io.Fonts->AddFontFromFileTTF("runtime/commodore_pet/font/PetMe.ttf", 10.0f, nullptr, ranges.Data);
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
	const ImVec2			size = {460, 220};
	std::string				title;
	float					footer_height;

	size_t					mem_size;
	size_t					mem_offset;

	int						display_type = 0;
	bool					follow_pc = false;

	char					input_addr[16] = {};
	int64_t					jump_addr = -1;

	static ImFont *			pet_font;
};

ImFont * PanelMemory::pet_font = nullptr;

Panel::uptr_t panel_memory_create(UIContext *ctx, struct ImVec2 pos, const char *title,
								  size_t data_offset, size_t data_size) {

	return std::make_unique<PanelMemory>(ctx, pos, title, data_size, data_offset);
}

void panel_memory_load_fonts() {
	PanelMemory::load_pet_font();
}
