cmake_minimum_required(VERSION 3.21)
project(Il2cppBase VERSION 2.0.0 LANGUAGES C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Build for x64. Unity IL2CPP games on Windows are 64-bit.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(VENDOR ${CMAKE_SOURCE_DIR}/vendor)
foreach(dep imgui minhook)
    if(NOT EXISTS ${VENDOR}/${dep})
        message(FATAL_ERROR "vendor/${dep} is missing. Run .\\build.ps1 once to fetch dependencies.")
    endif()
endforeach()

add_library(minhook STATIC
    ${VENDOR}/minhook/src/buffer.c
    ${VENDOR}/minhook/src/hook.c
    ${VENDOR}/minhook/src/trampoline.c
    ${VENDOR}/minhook/src/hde/hde64.c
)
target_include_directories(minhook PUBLIC ${VENDOR}/minhook/include PRIVATE ${VENDOR}/minhook/src)

add_library(imgui STATIC
    ${VENDOR}/imgui/imgui.cpp
    ${VENDOR}/imgui/imgui_draw.cpp
    ${VENDOR}/imgui/imgui_tables.cpp
    ${VENDOR}/imgui/imgui_widgets.cpp
    ${VENDOR}/imgui/backends/imgui_impl_win32.cpp
    ${VENDOR}/imgui/backends/imgui_impl_dx11.cpp
    ${VENDOR}/imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC ${VENDOR}/imgui ${VENDOR}/imgui/backends)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS IMGUI_DEFINE_MATH_OPERATORS)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
add_library(Il2cppBase SHARED ${SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/src FILES ${SOURCES})

target_include_directories(Il2cppBase PRIVATE src)
target_compile_definitions(Il2cppBase PRIVATE
    IL2CPP_BASE_VERSION="${PROJECT_VERSION}"
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
target_compile_options(Il2cppBase PRIVATE /W4 /permissive- /utf-8 /MP)
target_link_libraries(Il2cppBase PRIVATE imgui minhook d3d11 dxgi dbghelp)

set_target_properties(Il2cppBase PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)
