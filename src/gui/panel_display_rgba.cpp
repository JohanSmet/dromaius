// gui/panel_display_rgba.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_display_rgba.h"
#include "ui_context.h"

#include "display_rgba.h"
#include "imgui_ex.h"
#include <glad/glad.h>
#include <cmath>
#include <imgui.h>

class PanelDisplayRGBA : public Panel {
public:

	PanelDisplayRGBA(UIContext *ctx, ImVec2 pos, DisplayRGBA *display) :
		Panel(ctx),
		position(pos),
		display_rgba(display) {
		create_texture();
	}

	void init() override {
		// just an approximation for the first call to the constraints
		header_height = ImGuiEx::LabelHeight(txt_integer_scaling) + 8.0f;
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
		ImGui::SetNextWindowSizeConstraints(
				{0.0f, 0.0f}, {FLT_MAX, FLT_MAX},
				[](auto *data) {
					auto *panel = reinterpret_cast<PanelDisplayRGBA *>(data->UserData);
					auto w = static_cast<float>(panel->display_rgba->width);
					auto h = static_cast<float>(panel->display_rgba->height);
					data->DesiredSize.y = ((data->DesiredSize.x * h) / w) + panel->header_height;
				}, (void *) this
		);

		// display panel window
		if (ImGui::Begin(txt_title)) {
			if (ImGui::Checkbox(txt_integer_scaling, &integer_scaling)) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (integer_scaling) ? GL_NEAREST : GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (integer_scaling) ? GL_NEAREST : GL_LINEAR);
			}
			ImGui::Separator();
			ImGui::Spacing();

			auto region = ImGui::GetContentRegionAvail();
			auto w = static_cast<float>(display_rgba->width);
			auto h = static_cast<float>(display_rgba->height);

			// update to correct value so constraints can calculate correct aspect ratio
			header_height = ImGui::GetWindowHeight() - region.y;

			float scale = std::min(region.x / w, region.y / h);
			if (integer_scaling) {
				scale = std::floor(scale);
			}

			ImGui::Image((void *) (intptr_t) texture_id, ImVec2(w * scale, h * scale));
		}

		ImGui::End();
	}

private:
	void create_texture() {
		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id);

		// filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (integer_scaling) ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (integer_scaling) ? GL_NEAREST : GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// define texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					 static_cast<GLsizei>(display_rgba->width), static_cast<GLsizei>(display_rgba->height), 0,
					 GL_RGBA, GL_UNSIGNED_BYTE,
					 display_rgba->frame);
	}

private:
	static constexpr const char *txt_integer_scaling = "Integer Scaling";
	static constexpr const char *txt_title = "Display";

private:
	ImVec2			position;
	const ImVec2	size = {400, 300};
	DisplayRGBA *	display_rgba;
	GLuint			texture_id;

	float			header_height = 0.0f;
	bool			integer_scaling = true;

};

Panel::uptr_t panel_display_rgba_create(UIContext *ctx, struct ImVec2 pos, struct DisplayRGBA *display) {
	return std::make_unique<PanelDisplayRGBA>(ctx, pos, display);
}
