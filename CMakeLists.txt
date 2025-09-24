# Cross-compilable Godot .pck standalone extractor and packer
cmake_minimum_required(VERSION 3.10)

project(GodotPckTool)

set(GODOT_PCK_TOOL_VERSION_MAJOR 2)
set(GODOT_PCK_TOOL_VERSION_MINOR 2)

set(GODOT_PCK_TOOL_VERSION_STR "${GODOT_PCK_TOOL_VERSION_MAJOR}.${GODOT_PCK_TOOL_VERSION_MINOR}")

# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/CMake")
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET(BUILD_SHARED_LIBS OFF)

# Common options
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Set the build type, usually Release or RelWithDebInfo" FORCE)
endif(CMAKE_BUILD_TYPE STREQUAL "")

# Configure include file
configure_file("src/Include.h.in" "${PROJECT_BINARY_DIR}/Include.h")

include_directories(${PROJECT_BINARY_DIR})

add_subdirectory(third_party)
include_directories(third_party/cxxopts/include)
include_directories(third_party/json/single_include)

include_directories(src)
add_subdirectory(src)

# Install also the license files
install(FILES LICENSE LibraryLicenses.txt DESTINATION bin)
