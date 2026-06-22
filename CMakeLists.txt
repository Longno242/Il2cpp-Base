cmake_minimum_required(VERSION 3.20)
project(Il2cppBase LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Dependencies (run scripts/setup_deps.ps1 first)
# ---------------------------------------------------------------------------
set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")

if(NOT EXISTS "${VENDOR_DIR}/minhook/CMakeLists.txt")
    message(FATAL_ERROR "MinHook not found. Run: scripts/setup_deps.ps1")
endif()

if(NOT EXISTS "${VENDOR_DIR}/imgui/imgui.h")
    message(FATAL_ERROR "Dear ImGui not found. Run: scripts/setup_deps.ps1")
endif()

add_subdirectory("${VENDOR_DIR}/minhook" EXCLUDE_FROM_ALL)

set(IMGUI_DIR "${VENDOR_DIR}/imgui")
set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp"
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

# ---------------------------------------------------------------------------
# Mod DLL
# ---------------------------------------------------------------------------
file(GLOB_RECURSE MOD_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_library(Il2cppBase SHARED ${MOD_SOURCES})
target_include_directories(Il2cppBase PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(Il2cppBase PRIVATE
    minhook
    imgui
    d3d11
    dxgi
)

target_compile_definitions(Il2cppBase PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    IL2CPP_BASE_EXPORTS
)

if(MSVC)
    target_compile_options(Il2cppBase PRIVATE /W4 /permissive-)
endif()

set_target_properties(Il2cppBase PROPERTIES
    OUTPUT_NAME "Il2cppBase"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
