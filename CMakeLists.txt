cmake_minimum_required(VERSION 3.10)
project(Vita3K)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_GDBSTUB "Build Vita3K with GDB Debugger." OFF)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

enable_testing()

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
	add_definitions (/D "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
endif()

add_subdirectory(src/external)
add_subdirectory(src/emulator)
add_subdirectory(src/gen-modules)
