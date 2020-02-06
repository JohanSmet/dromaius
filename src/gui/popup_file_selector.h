// gui/popup_file_selector.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_POPUP_FILE_SELECTOR_H
#define DROMAIUS_GUI_POPUP_FILE_SELECTOR_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

class PopupFileSelector {
public:
	using uptr_t = std::unique_ptr<PopupFileSelector>;
	using on_select_callback_t = std::function<void (const std::string &)>;
public:
	PopupFileSelector(class UIContext *ctx, const char *path, const char *extension, const char *prefix);

	void define_popup();
	void display_popup(on_select_callback_t on_select);

	std::vector<std::string> construct_directory_listing();

	static uptr_t make_unique(class UIContext *ctx, const char *path, const char *extension, const char *prefix) {
		return std::make_unique<PopupFileSelector>(ctx, path, extension, prefix);
	}

private:
	std::vector<std::string>	filenames;
	std::string					title;
	std::string					path;
	std::string					extension;
	std::string					prefix;

	on_select_callback_t		on_select_callback = nullptr;
};


#endif // DROMAIUS_GUI_POPUP_FILE_SELECTOR_H
