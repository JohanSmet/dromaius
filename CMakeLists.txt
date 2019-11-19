cmake_minimum_required(VERSION 3.12)

project(dromaius)

# force C11 for all targets
set(CMAKE_C_STANDARD 11)

# export a JSON compilation database for clangd
set (CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

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
		unset(HAVE_FLOOR CACHE)
	endif()
endif()

# external libraries
set (STB_TARGET stb)
add_library(${STB_TARGET} STATIC)
target_sources(${STB_TARGET}
	PRIVATE
		libs/stb/stb_ds.h
		libs/stb/stb_impl.c
)

# simulator library
set (LIB_TARGET dromaius)

add_library(${LIB_TARGET} STATIC)
target_sources(${LIB_TARGET}
	PRIVATE
		src/cpu_6502.c
		src/cpu_6502.h
		src/dev_minimal_6502.c
		src/dev_minimal_6502.h
		src/ram_8d_16a.c
		src/ram_8d_16a.h
		src/rom_8d_16a.c
		src/rom_8d_16a.h
		src/utils.c
		src/utils.h
)
target_include_directories(${LIB_TARGET} PRIVATE libs)

# unit tests
enable_testing()

set (TEST_TARGET test_runner)

add_executable(${TEST_TARGET})
target_sources(${TEST_TARGET}
	PRIVATE
		src/test/test_main.c
		src/test/test_cpu_6502.c
		src/test/test_dev_minimal_6502.c
		src/test/test_ram_8d_16a.c
		src/test/test_rom_8d_16a.c
		libs/munit/munit.c
		libs/munit/munit.h
)
target_include_directories(${TEST_TARGET} PRIVATE libs src)
target_link_libraries(${TEST_TARGET} PRIVATE ${LIB_TARGET})
target_link_libraries(${TEST_TARGET} PRIVATE ${STB_TARGET})
target_link_libraries(${TEST_TARGET} PRIVATE ${LIBS_MATH})

add_test(NAME unittests COMMAND ${TEST_TARGET})

