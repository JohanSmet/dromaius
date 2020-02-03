cmake_minimum_required(VERSION 3.12)

project(dromaius)

# options
option (NO_THREADING "Disable multithreading in the Dromaius context" OFF)

# force C11 for all targets
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# export a JSON compilation database for clangd
set (CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# platform detection
string(TOUPPER ${CMAKE_SYSTEM_NAME} PLATFORM_NAME)
string(CONCAT PLATFORM_DEF "PLATFORM_" ${PLATFORM_NAME})

# check system libraries
include(CheckSymbolExists)

# >> some platform require an extra link-libraries for math functions and others do not
check_symbol_exists("floor" "math.h" HAVE_FLOOR)
if (NOT HAVE_FLOOR)
	unset(HAVE_FLOOR CACHE)
	list(APPEND CMAKE_REQUIRED_LIBRARIES m)
	check_symbol_exists("floor" "math.h" HAVE_FLOOR)
	if (NOT HAVE_FLOOR)
		message(FATAL_ERROR "Unable to find math functions")
	else()
		list(APPEND LIBS_MATH m)
		list(REMOVE_ITEM CMAKE_REQUIRED_LIBRARIES m)
		unset(HAVE_FLOOR CACHE)
	endif()
endif()

# external library - single-file libraries
set (SFL_TARGET stb)
add_library(${SFL_TARGET} STATIC)
target_sources(${SFL_TARGET}
	PRIVATE
		libs/cute/cute_files.c
		libs/cute/cute_files.h
		libs/stb/stb_ds.h
		libs/stb/stb_impl.c
)

# external library - glfw
if (NOT EMSCRIPTEN)
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

	add_subdirectory(libs/glfw)
endif()

# external library - OpenGL
if (NOT EMSCRIPTEN)
	find_package(OpenGL REQUIRED)
endif()

# external library Dear Imgui
set (IMGUI_TARGET imgui)
add_library(${IMGUI_TARGET} STATIC)
target_sources(${IMGUI_TARGET}
	PRIVATE
		libs/glad/glad.c
		libs/glad/glad.h
		libs/glad/KHR/khrplatform.h
		libs/imgui/imconfig.h
		libs/imgui/imgui.cpp
		libs/imgui/imgui.h
		libs/imgui/imgui_draw.cpp
		libs/imgui/imgui_internal.h
		libs/imgui/imstb_rectpack.h
		libs/imgui/imstb_textedit.h
		libs/imgui/imstb_truetype.h
		libs/imgui/imgui_widgets.cpp
		libs/imgui/examples/imgui_impl_glfw.cpp
		libs/imgui/examples/imgui_impl_glfw.h
		libs/imgui/examples/imgui_impl_opengl3.cpp
		libs/imgui/examples/imgui_impl_opengl3.h
)
target_include_directories(${IMGUI_TARGET} PUBLIC libs libs/imgui libs/glad)
target_include_directories(${IMGUI_TARGET} PRIVATE ${OPENGL_INCLUDE_DIR})
target_link_libraries(${IMGUI_TARGET} PUBLIC glfw)
target_link_libraries(${IMGUI_TARGET} PRIVATE ${OPENGL_LIBRARIES})

# simulator library
set (LIB_TARGET dromaius)

add_library(${LIB_TARGET} STATIC)
target_sources(${LIB_TARGET}
	PRIVATE
		src/sys/threads.c
		src/sys/threads.h
		src/chip_6520.c
		src/chip_6520.h
		src/chip_hd44780.c
		src/chip_hd44780.h
		src/clock.c
		src/clock.h
		src/context.c
		src/context.h
		src/cpu_6502.c
		src/cpu_6502.h
		src/dev_minimal_6502.c
		src/dev_minimal_6502.h
		src/filt_6502_asm.c
		src/filt_6502_asm.h
		src/ram_8d_16a.c
		src/ram_8d_16a.h
		src/rom_8d_16a.c
		src/rom_8d_16a.h
		src/signal_line.c
		src/signal_line.h
		src/utils.c
		src/utils.h
)
target_include_directories(${LIB_TARGET} PRIVATE libs)
target_link_libraries(${LIB_TARGET} PRIVATE ${SFL_TARGET})
target_link_libraries(${LIB_TARGET} PRIVATE ${LIBS_MATH})
target_compile_definitions(${LIB_TARGET} PRIVATE ${PLATFORM_DEF})
if (NO_THREADING OR EMSCRIPTEN)
	target_compile_definitions(${LIB_TARGET} PUBLIC DMS_NO_THREADING)
endif()

# gui
set (GUI_TARGET dromaius_gui)

add_executable(${GUI_TARGET} WIN32)
target_sources(${GUI_TARGET}
	PRIVATE
		src/gui/imgui_ex.cpp
		src/gui/imgui_ex.h
		src/gui/main.cpp
		src/gui/main_gui.cpp
		src/gui/panel.h
		src/gui/panel_chip_6520.cpp
		src/gui/panel_chip_6520.h
		src/gui/panel_chip_hd44780.cpp
		src/gui/panel_chip_hd44780.h
		src/gui/panel_clock.cpp
		src/gui/panel_clock.h
		src/gui/panel_control.cpp
		src/gui/panel_control.h
		src/gui/panel_cpu_6502.cpp
		src/gui/panel_cpu_6502.h
		src/gui/panel_memory.cpp
		src/gui/panel_memory.h
		src/gui/panel_monitor.cpp
		src/gui/panel_monitor.h
		src/gui/ui_context.h
		src/gui/widgets.h
)
target_include_directories(${GUI_TARGET} PRIVATE libs src)
target_link_libraries(${GUI_TARGET} PRIVATE ${IMGUI_TARGET})
target_link_libraries(${GUI_TARGET} PRIVATE ${LIB_TARGET})

if (WIN32)
	target_link_options(${GUI_TARGET} PRIVATE "/ENTRY:mainCRTStartup")
endif()

if (EMSCRIPTEN) 
	target_link_libraries(${GUI_TARGET} PRIVATE 
		"-s USE_GLFW=3" 
		"-s WASM=1"
		"-s ALLOW_MEMORY_GROWTH=1"
		"-s ASSERTIONS=2"
		"-s USE_WEBGL2=1"
		"-s FULL_ES3=1"
		"--no-heap-copy"
	)
	target_link_libraries(${GUI_TARGET} PRIVATE
		"--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/emscripten_minimal.html"
		"-o dromaius.html"
	)
	target_link_libraries(${GUI_TARGET} PRIVATE
		"--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/runtime@/runtime"
			"--exclude-file *Makefile"
			"--exclude-file *.gitignore"
			"--exclude-file *.s"
			"--exclude-file *.cfg"
	)
endif()

# unit tests
enable_testing()

set (TEST_TARGET test_runner)

add_executable(${TEST_TARGET})
target_sources(${TEST_TARGET}
	PRIVATE
		src/test/test_main.c
		src/test/test_chip_6520.c
		src/test/test_chip_hd44780.c
		src/test/test_clock.c
		src/test/test_cpu_6502.c
		src/test/test_dev_minimal_6502.c
		src/test/test_filt_6502_asm.c
		src/test/test_ram_8d_16a.c
		src/test/test_rom_8d_16a.c
		src/test/test_signal_line.c
		src/test/test_utils.c
		libs/munit/munit.c
		libs/munit/munit.h
)
target_include_directories(${TEST_TARGET} PRIVATE libs src)
target_link_libraries(${TEST_TARGET} PRIVATE ${LIB_TARGET})

add_test(NAME unittests COMMAND ${TEST_TARGET})

