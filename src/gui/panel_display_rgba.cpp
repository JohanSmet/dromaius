// gui/panel_display_rgba.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_display_rgba.h"
#include "ui_context.h"

#include "display_rgba.h"
#include "imgui_ex.h"
#include <glad/glad.h>
#include <cmath>

class PanelDisplayRGBA : public Panel {
public:

	PanelDisplayRGBA(UIContext *ctx, ImVec2 pos, DisplayRGBA *display) :
		Panel(ctx),
		position(pos),
		display_rgba(display) {
		create_texture();
	}

	void display() override {
		// update texture
		glBindTexture(GL_TEXTURE_2D, texture_id);
		glTexSubImage2D(GL_TEXTURE_2D, 0,
						0, 0, static_cast<GLsizei>(display_rgba->width), static_cast<GLsizei>(display_rgba->height),
						GL_RGBA, GL_UNSIGNED_BYTE,
						display_rgba->frame);

		// set window properties
		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		// display panel window
		if (ImGui::Begin(title)) {
			auto region = ImGui::GetContentRegionAvail();
			auto w = static_cast<float>(display_rgba->width);
			auto h = static_cast<float>(display_rgba->height);

			float scale = std::min(region.x / w, region.y / h);

			ImGui::Image((void *) (intptr_t) texture_id, ImVec2(w * scale, h * scale));
		}

		ImGui::End();
	}

private:
	void create_texture() {
		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id);

		// filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// define texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					 static_cast<GLsizei>(display_rgba->width), static_cast<GLsizei>(display_rgba->height), 0,
					 GL_RGBA, GL_UNSIGNED_BYTE,
					 display_rgba->frame);
	}

private:
	ImVec2			position;
	const ImVec2	size = {330, 0};
	DisplayRGBA *	display_rgba;
	GLuint			texture_id;

	constexpr static const char *title = "Display";
};

Panel::uptr_t panel_display_rgba_create(UIContext *ctx, struct ImVec2 pos, struct DisplayRGBA *display) {
	return std::make_unique<PanelDisplayRGBA>(ctx, pos, display);
}
