cmake_minimum_required(VERSION 3.16)
project(qefir)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

# Find ALSA
find_package(ALSA REQUIRED)

# Include directories
include_directories(
    .
    include
    include/nodes
    include/filters
)

# Automatically collect all source files
file(GLOB SRC_FILES
    qefir.cpp
    src/*.cpp
    src/nodes/*.cpp
    src/filters/*.cpp
)

# Executable
add_executable(qefir ${SRC_FILES})

# Link libraries
target_link_libraries(qefir ALSA::ALSA)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)