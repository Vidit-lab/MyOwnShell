cmake_minimum_required(VERSION 3.16)

project(myshell
  VERSION 0.1.0
  DESCRIPTION "A POSIX-style shell written from scratch in C++"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for clangd / IDE tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_executable(myshell
  src/main.cpp
)

target_include_directories(myshell PRIVATE src)

target_compile_options(myshell PRIVATE
  -Wall
  -Wextra
)

# Put the binary at the top of the build tree so tests/run_tests.sh finds it.
set_target_properties(myshell PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

enable_testing()
add_subdirectory(tests)
