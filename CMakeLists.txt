cmake_minimum_required(VERSION 3.12)

project(dromaius)

# force C11 for all targets
set(CMAKE_C_STANDARD 11)

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
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(libs/glfw)

# external library - OpenGL
find_package(OpenGL REQUIRED)

# external library - nuklear
set (NUKLEAR_TARGET nuklear)
add_library(${NUKLEAR_TARGET} STATIC)
target_sources(${NUKLEAR_TARGET}
	PRIVATE
		libs/glad/glad.c
		libs/glad/glad.h
		libs/glad/KHR/khrplatform.h
		libs/nuklear/main.c
		libs/nuklear/nuklear.h
		libs/nuklear/nuklear_std.h
		libs/nuklear/nuklear_glfw_gl3.h
)

target_include_directories(${NUKLEAR_TARGET} PRIVATE libs libs/nuklear)
target_include_directories(${NUKLEAR_TARGET} PRIVATE ${OPENGL_INCLUDE_DIR})
target_link_libraries(${NUKLEAR_TARGET} PRIVATE glfw)
target_link_libraries(${NUKLEAR_TARGET} PRIVATE ${OPENGL_LIBRARIES})

# simulator library
set (LIB_TARGET dromaius)

add_library(${LIB_TARGET} STATIC)
target_sources(${LIB_TARGET}
	PRIVATE
		src/sys/threads.c
		src/sys/threads.h
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
		src/utils.c
		src/utils.h
)
target_include_directories(${LIB_TARGET} PRIVATE libs)
target_link_libraries(${LIB_TARGET} PRIVATE ${SFL_TARGET})
target_link_libraries(${LIB_TARGET} PRIVATE ${LIBS_MATH})
target_compile_definitions(${LIB_TARGET} PRIVATE ${PLATFORM_DEF})

# gui
set (GUI_TARGET dromaius_gui)

add_executable(${GUI_TARGET})
target_sources(${GUI_TARGET}
	PRIVATE
		src/gui/main_gui.c
		src/gui/panel_clock.c
		src/gui/panel_clock.h
		src/gui/panel_control.c
		src/gui/panel_control.h
		src/gui/panel_cpu_6502.c
		src/gui/panel_cpu_6502.h
		src/gui/panel_memory.c
		src/gui/panel_memory.h
		src/gui/panel_monitor.c
		src/gui/panel_monitor.h
		src/gui/ui_context.h
)
target_include_directories(${GUI_TARGET} PRIVATE libs src)
target_link_libraries(${GUI_TARGET} PRIVATE ${NUKLEAR_TARGET})
target_link_libraries(${GUI_TARGET} PRIVATE ${LIB_TARGET})

# unit tests
enable_testing()

set (TEST_TARGET test_runner)

add_executable(${TEST_TARGET})
target_sources(${TEST_TARGET}
	PRIVATE
		src/test/test_main.c
		src/test/test_clock.c
		src/test/test_cpu_6502.c
		src/test/test_dev_minimal_6502.c
		src/test/test_filt_6502_asm.c
		src/test/test_ram_8d_16a.c
		src/test/test_rom_8d_16a.c
		src/test/test_utils.c
		libs/munit/munit.c
		libs/munit/munit.h
)
target_include_directories(${TEST_TARGET} PRIVATE libs src)
target_link_libraries(${TEST_TARGET} PRIVATE ${LIB_TARGET})

add_test(NAME unittests COMMAND ${TEST_TARGET})

