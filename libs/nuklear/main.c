/* nuklear - 1.32.0 - public domain */
#include <stdarg.h>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif // __EMSCRIPTEN__

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

/*****************************************************************************
 * STYLES 
 ****************************************************************************/ 

#define INCLUDE_STYLE

#ifdef INCLUDE_STYLE
  #include "style.c"
#endif

/*****************************************************************************
 * EXTERNAL FUNCTIONS
 ****************************************************************************/ 

char *nuklear_config_window_title(void);
void nuklear_on_start(struct nk_context *ctx);
void nuklear_gui(struct nk_context *ctx);

/*****************************************************************************
 * IMPLEMENTATION
 ****************************************************************************/ 

static void error_callback(int e, const char *d) {
	printf("GLFW Error %d: %s\n", e, d);
}

/* Platform */
static GLFWwindow *win;
static struct nk_context *ctx;

void main_loop(void *arg);

int main(void)
{
    int width = WINDOW_WIDTH;
	int height = WINDOW_HEIGHT;

    /* GLFW */
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        fprintf(stdout, "[GFLW] failed to init!\n");
        exit(1);
    }

#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	width = EM_ASM_INT_V(return window.innerWidth);
	height = EM_ASM_INT_V(return window.innerHeight);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
#endif
    win = glfwCreateWindow(width, height, nuklear_config_window_title(), NULL, NULL);
    glfwMakeContextCurrent(win);

	/* get width & height of the content area */
    glfwGetWindowSize(win, &width, &height);

    /* OpenGL */
#ifdef __EMSCRIPTEN__
	if (!gladLoadGLES2Loader((GLADloadproc) glfwGetProcAddress)) {
#else
	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
#endif
        fprintf(stderr, "Failed to setup glad\n");
        exit(1);
	}

	printf("%dx%d\n", width, height);

    glViewport(0, 0, width, height);

    ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&atlas);
    /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
    /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 14, 0);*/
    /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
    /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
    /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
    /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
    nk_glfw3_font_stash_end();
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    /*nk_style_set_font(ctx, &droid->handle);*/}

    #ifdef INCLUDE_STYLE
    /*set_style(ctx, THEME_WHITE);*/
    /*set_style(ctx, THEME_RED);*/
    /*set_style(ctx, THEME_BLUE);*/
    /*set_style(ctx, THEME_DARK);*/
    #endif

	nuklear_on_start(ctx);

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(main_loop, NULL, 0, 1);
#else 
    while (!glfwWindowShouldClose(win)) {
		main_loop(NULL);
    }
    nk_glfw3_shutdown();
    glfwTerminate();
#endif
    return 0;

}

void main_loop(void *arg) {

    struct nk_colorf bg = {.r = 0.10f, .g = 0.18f, .b = 0.24f, .a = 1.0f};
    int width = 0, height = 0;

	/* Input */
	glfwPollEvents();
	nk_glfw3_new_frame();

	/* GUI */
	nuklear_gui(ctx);

	/* Draw */
	glfwGetWindowSize(win, &width, &height);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(bg.r, bg.g, bg.b, bg.a);
	/* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
	 * with blending, scissor, face culling, depth test and viewport and
	 * defaults everything back into a default state.
	 * Make sure to either a.) save and restore or b.) reset your own state after
	 * rendering the UI. */
	nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
	glfwSwapBuffers(win);
}

