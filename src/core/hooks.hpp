#pragma once

#include <MinHook.h>

#include <mutex>
#include <string>
#include <unordered_map>

#include "core/logger.hpp"

namespace il2cpp_base::hooks {

namespace detail {

struct Hook {
    void* target = nullptr;
    bool enabled = false;
};

struct State {
    std::mutex mutex;
    bool initialized = false;
    std::unordered_map<std::string, Hook> hooks;
};

inline State& state() {
    static State s;
    return s;
}

inline bool install_raw(const char* name, void* target, void* detour, void** original) {
    if (!name || !target || !detour || !original) {
        LOG_ERROR("Hooks", "bad arguments for '%s'", name ? name : "?");
        return false;
    }

    auto& s = state();
    std::lock_guard lock(s.mutex);
    if (!s.initialized) {
        LOG_ERROR("Hooks", "hooks::init() was not called");
        return false;
    }
    if (s.hooks.contains(name)) {
        LOG_WARN("Hooks", "'%s' already installed", name);
        return false;
    }

    if (const MH_STATUS st = MH_CreateHook(target, detour, original); st != MH_OK) {
        LOG_ERROR("Hooks", "create '%s' failed: %s", name, MH_StatusToString(st));
        return false;
    }
    if (const MH_STATUS st = MH_EnableHook(target); st != MH_OK) {
        LOG_ERROR("Hooks", "enable '%s' failed: %s", name, MH_StatusToString(st));
        MH_RemoveHook(target);
        return false;
    }

    s.hooks[name] = Hook{ target, true };
    LOG_INFO("Hooks", "installed '%s' @ %p", name, target);
    return true;
}

} // namespace detail

inline bool init() {
    auto& s = detail::state();
    std::lock_guard lock(s.mutex);
    if (s.initialized) {
        return true;
    }
    const MH_STATUS st = MH_Initialize();
    if (st != MH_OK && st != MH_ERROR_ALREADY_INITIALIZED) {
        LOG_ERROR("Hooks", "MH_Initialize failed: %s", MH_StatusToString(st));
        return false;
    }
    s.initialized = true;
    return true;
}

inline void shutdown() {
    auto& s = detail::state();
    std::lock_guard lock(s.mutex);
    if (!s.initialized) {
        return;
    }
    for (auto& [_, hook] : s.hooks) {
        MH_DisableHook(hook.target);
        MH_RemoveHook(hook.target);
    }
    s.hooks.clear();
    MH_Uninitialize();
    s.initialized = false;
}

// Creates and enables a hook. `original` receives a pointer that calls the unhooked function.
template <typename Fn>
bool install(const char* name, void* target, Fn detour, Fn* original) {
    return detail::install_raw(name, target, reinterpret_cast<void*>(detour), reinterpret_cast<void**>(original));
}

inline bool set_enabled(const char* name, bool enabled) {
    auto& s = detail::state();
    std::lock_guard lock(s.mutex);
    const auto it = s.hooks.find(name);
    if (it == s.hooks.end()) {
        return false;
    }
    if (it->second.enabled == enabled) {
        return true;
    }
    const MH_STATUS st = enabled ? MH_EnableHook(it->second.target) : MH_DisableHook(it->second.target);
    if (st != MH_OK) {
        LOG_ERROR("Hooks", "toggle '%s' failed: %s", name, MH_StatusToString(st));
        return false;
    }
    it->second.enabled = enabled;
    return true;
}

inline bool is_enabled(const char* name) {
    auto& s = detail::state();
    std::lock_guard lock(s.mutex);
    const auto it = s.hooks.find(name);
    return it != s.hooks.end() && it->second.enabled;
}

inline bool remove(const char* name) {
    auto& s = detail::state();
    std::lock_guard lock(s.mutex);
    const auto it = s.hooks.find(name);
    if (it == s.hooks.end()) {
        return false;
    }
    MH_DisableHook(it->second.target);
    MH_RemoveHook(it->second.target);
    s.hooks.erase(it);
    return true;
}

} // namespace il2cpp_base::hooks
