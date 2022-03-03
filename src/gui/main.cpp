// main application entry point (native + wasm), based upon
// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline

#include "imgui.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "gui/ui_context.h"

#include <argh/argh.h>
#include <cstdlib>
#include <cstdio>
#include <memory>

#include <glad/glad.h>  // Initialize with gladLoadGL()

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

EM_BOOL emscripten_handle_resize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData);
#endif // __EMSCRIPTEN__

namespace {

// Emscripten requires full control of the main loop. Store GLFW book-keeping variables globally.
GLFWwindow *g_window = nullptr;
std::unique_ptr<UIContext> g_ui_context = nullptr;

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

using argh_list_t = std::initializer_list<const char *const>;
static constexpr argh_list_t ARG_MACHINE = {"-m", "--machine"};
static constexpr argh_list_t ARG_HELP = {"-h", "--help"};

void print_help() {
	auto format_argh_list = [](const argh_list_t &args) -> auto {
		std::string result;
		const char *sepa = "";

		for (const auto &a : args) {
			result.append(sepa);
			result.append(a);
			sepa = ", ";
		}

		return result;
	};

	printf("Usage:\n\n");
	printf("dromaius_gui [options]\n\n");
	printf("Options\n");
	printf(" %-25s specify machine to emulate (commodore-pet, commodore-pet-lite, minimal-6502).\n",
			format_argh_list(ARG_MACHINE).c_str());
	printf(" %-25s display this help message and stop execution.\n",
			format_argh_list(ARG_HELP).c_str());
}

} // unnamed namespace

///////////////////////////////////////////////////////////////////////////////
//
// interface
//

void main_loop(void *arg);

int main(int argc, char **argv)
{
	double width = 1200;
	double height = 800;

	// parse command-line arguments
	argh::parser cmd_line;
	cmd_line.add_params(ARG_MACHINE);
	cmd_line.add_params(ARG_HELP);
	cmd_line.parse(argc, argv);

	if (cmd_line[ARG_HELP]) {
		print_help();
		exit(EXIT_SUCCESS);
	}

	// fill-out configuration structure
	Config ui_config;

	{
		std::string machine;
		cmd_line(ARG_MACHINE, "commodore-pet") >> machine;

		if (!ui_config.set_machine_type(machine.c_str())) {
			fprintf(stderr, "Invalid machine type specified (%s)\n", machine.c_str());
			exit(EXIT_FAILURE);
		}
	}

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#elif __EMSCRIPTEN__
    const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	emscripten_get_element_css_size("canvas", &width, &height);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    g_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), "Dromaius", NULL, NULL);
    if (g_window == NULL)
        return 1;
    glfwMakeContextCurrent(g_window);

#ifndef __EMSCRIPTEN__
    glfwSwapInterval(1); // Enable vsync
#endif

    // Initialize OpenGL loader
#ifdef __EMSCRIPTEN__
	bool err = gladLoadGLES2Loader((GLADloadproc) glfwGetProcAddress) == 0;
#else
	bool err = gladLoadGL() == 0;
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;			// Enable Docking
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	 //io.ConfigViewportsNoAutoMerge = true;
	 //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont()
	//   to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application
	//   (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
	//   ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

	// initialize application
	g_ui_context = std::make_unique<UIContext>(ui_config);
	g_ui_context->setup_ui(g_window);

    // main loop
#ifdef __EMSCRIPTEN__
	emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 0, emscripten_handle_resize);
	emscripten_set_main_loop_arg(main_loop, nullptr, 0, 1);
#else
    while (!glfwWindowShouldClose(g_window)) {
		main_loop(nullptr);
    }
#endif

    // cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(g_window);
    glfwTerminate();

    return 0;
}

void main_loop([[maybe_unused]] void *arg) {

    ImGuiIO& io = ImGui::GetIO();
    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	glfwPollEvents();

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// application specific stuff
	g_ui_context->draw_ui();

	//ImGui::ShowDemoWindow();
	//ImGui::ShowMetricsWindow();

	// Rendering
	ImGui::Render();
	glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}

	glfwSwapBuffers(g_window);
}

#ifdef __EMSCRIPTEN__

EM_BOOL emscripten_handle_resize([[maybe_unused]] int eventType, [[maybe_unused]] const EmscriptenUiEvent *uiEvent, [[maybe_unused]] void *userData) {
	double w, h;

	emscripten_get_element_css_size("canvas", &w, &h);
	glfwSetWindowSize(g_window, (int) w, (int) h);

	return 0;
}

#endif // __EMSCRIPTEN__
