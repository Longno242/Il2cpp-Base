#include <Windows.h>

#include <imgui.h>

#include <filesystem>
#include <string>
#include <thread>

#include "il2cpp_base/core/config.hpp"
#include "il2cpp_base/core/hooks.hpp"
#include "il2cpp_base/core/logger.hpp"
#include "il2cpp_base/il2cpp/resolver.hpp"
#include "il2cpp_base/ui/overlay.hpp"

namespace {

using namespace il2cpp_base;

std::wstring log_path() {
    wchar_t module_path[MAX_PATH]{};
    GetModuleFileNameW(GetModuleHandleW(nullptr), module_path, MAX_PATH);
    const auto dir = std::filesystem::path(module_path).parent_path();
    return (dir / config::kLogFileName).wstring();
}

void wait_for_module(const char* name, DWORD timeout_ms = 120000) {
    const DWORD step = 250;
    DWORD waited = 0;
    while (!GetModuleHandleA(name) && waited < timeout_ms) {
        Sleep(step);
        waited += step;
    }
}

void draw_menu() {
    ImGui::Text("IL2CPP resolver: %s", il2cpp::ready() ? "ready" : "not ready");

    if (il2cpp::ready()) {
        ImGui::Text("Assemblies: %zu", il2cpp::images().size());

        if (ImGui::BeginChild("assemblies", ImVec2(0, 120), true)) {
            for (const il2cpp::Image* img : il2cpp::images()) {
                ImGui::Text("%s", img->name().c_str());
            }
        }
        ImGui::EndChild();
    }

    ImGui::Separator();
    ImGui::TextDisabled("INSERT toggles this menu");
    ImGui::TextDisabled("Edit src/dllmain.cpp to add your mod logic");
}

DWORD WINAPI main_thread(LPVOID module) {
    log::Logger::instance().init(log_path());
    LOG_INFO("Core", "Il2cppBase loaded");

    wait_for_module(config::kGameAssemblyModule);
    if (!GetModuleHandleA(config::kGameAssemblyModule)) {
        LOG_FATAL("Core", "Timed out waiting for %s", config::kGameAssemblyModule);
        FreeLibraryAndExitThread(static_cast<HMODULE>(module), 1);
        return 1;
    }

    if (!il2cpp::init()) {
        LOG_FATAL("Core", "IL2CPP resolver failed — check patterns for this game build");
    }

    ui::Overlay::instance().set_render_callback(draw_menu);
    if (!ui::Overlay::instance().init()) {
        LOG_FATAL("Core", "Overlay init failed");
    }

    LOG_INFO("Core", "Initialization complete");
    return 0;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        const auto handle = CreateThread(nullptr, 0, main_thread, module, 0, nullptr);
        if (handle) {
            CloseHandle(handle);
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        il2cpp_base::ui::Overlay::instance().shutdown();
        il2cpp_base::hooks::HookManager::instance().shutdown();
        il2cpp_base::log::Logger::instance().shutdown();
    }
    return TRUE;
}
