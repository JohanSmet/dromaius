cmake_minimum_required(VERSION 3.12)

project(dromaius)

# force C11 for all targets
set(CMAKE_C_STANDARD 11)

# export a JSON compilation database for clangd
set (CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# simulator library
set (LIB_TARGET dromaius)

add_library(${LIB_TARGET} STATIC)
target_sources(${LIB_TARGET}
	PRIVATE
		src/cpu_6502.c
		src/cpu_6502.h
)

# unit tests
enable_testing()

set (TEST_TARGET test_runner)

add_executable(${TEST_TARGET})
target_sources(${TEST_TARGET}
	PRIVATE
		src/test/test_main.c
		src/test/test_cpu_6502.c
		libs/munit/munit.c
		libs/munit/munit.h
)
target_include_directories(${TEST_TARGET} PRIVATE libs src)
target_link_libraries(${TEST_TARGET} PRIVATE ${LIB_TARGET})

add_test(NAME unittests COMMAND ${TEST_TARGET})

