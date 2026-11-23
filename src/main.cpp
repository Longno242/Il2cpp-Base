#include <Windows.h>

#include <filesystem>
#include <string>

#include "config.hpp"
#include "core/crash.hpp"
#include "core/hooks.hpp"
#include "core/logger.hpp"
#include "core/main_thread.hpp"
#include "features/features.hpp"
#include "il2cpp/resolver.hpp"
#include "ui/cursor.hpp"
#include "ui/overlay.hpp"

namespace {

using namespace il2cpp_base;

std::wstring log_path() {
    wchar_t module_path[MAX_PATH]{};
    GetModuleFileNameW(GetModuleHandleW(nullptr), module_path, MAX_PATH);
    return (std::filesystem::path(module_path).parent_path() / config::kLogFileName).wstring();
}

bool wait_for_module(const char* name, DWORD timeout_ms) {
    for (DWORD waited = 0; !GetModuleHandleA(name); waited += 250) {
        if (waited >= timeout_ms) {
            return false;
        }
        Sleep(250);
    }
    return true;
}

DWORD WINAPI start(LPVOID) {
    log::Logger::instance().init(log_path(), config::kShowConsole);
    crash::install();
    LOG_INFO("Core", "%s v%s loaded", config::kName, IL2CPP_BASE_VERSION);

    if (!wait_for_module(config::kGameAssemblyModule, 120000)) {
        LOG_FATAL("Core", "timed out waiting for %s", config::kGameAssemblyModule);
        return 1;
    }
    wait_for_module(config::kUnityPlayerModule, 15000);

    const bool il2cpp_ready = il2cpp::init();
    if (!il2cpp_ready) {
        LOG_FATAL("Core", "IL2CPP resolver failed, see the missing exports above");
    }

    if (!ui::overlay::install()) {
        LOG_ERROR("Core", "overlay failed to install, running without a menu");
        if (il2cpp_ready) {
            il2cpp::ensure_thread();
            features::install();
        }
        return 0;
    }

    if (il2cpp_ready) {
        main_thread::post([] {
            ui::cursor::install();
            features::install();
            LOG_INFO("Core", "ready, press %s to open the menu", ui::overlay::menu_key_name());
        });
    }
    return 0;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        if (HANDLE thread = CreateThread(nullptr, 0, start, nullptr, 0, nullptr)) {
            CloseHandle(thread);
        }
    } else if (reason == DLL_PROCESS_DETACH && reserved == nullptr) {
        // Only on FreeLibrary. When the process is exiting the game is already torn down.
        features::shutdown();
        ui::overlay::uninstall();
        hooks::shutdown();
        crash::shutdown();
        log::Logger::instance().shutdown();
    }
    return TRUE;
}
