#include "il2cpp_base/features/example_hook.hpp"

#include <atomic>
#include <cstdio>

#include <imgui.h>

#include "il2cpp_base/core/config.hpp"
#include "il2cpp_base/core/hooks.hpp"
#include "il2cpp_base/core/logger.hpp"
#include "il2cpp_base/il2cpp/resolver.hpp"
#include "il2cpp_base/ui/theme.hpp"

namespace il2cpp_base::features {
namespace {

std::atomic<bool> g_installed{ false };
std::atomic<bool> g_hook_enabled{ false };
std::atomic<std::uint64_t> g_call_count{ 0 };

il2cpp::Method* g_target_method = nullptr;

using example_fn = void(*)();
example_fn g_original = nullptr;

bool game_config_ready() {
    return game::kExampleAssembly[0] != '\0' &&
           game::kExampleClass[0] != '\0' &&
           game::kExampleMethod[0] != '\0';
}

void hooked_example() {
    g_call_count.fetch_add(1, std::memory_order_relaxed);
    if (g_original) {
        g_original();
    }
}

} // namespace

bool install() {
    if (g_installed.load()) {
        return true;
    }

    if (!game_config_ready()) {
        LOG_INFO("ExampleHook", "skipped (kExample* empty)");
        g_installed.store(true);
        return true;
    }

    if (!il2cpp::ready()) {
        LOG_WARN("ExampleHook", "il2cpp not ready");
        return false;
    }

    il2cpp::Class* klass = il2cpp::find_class(
        game::kExampleAssembly, game::kExampleClass, game::kExampleNamespace);
    if (!klass) {
        LOG_ERROR("ExampleHook", "class missing: %s.%s", game::kExampleNamespace, game::kExampleClass);
        return false;
    }

    g_target_method = klass->method(game::kExampleMethod, game::kExampleMethodArgCount);
    if (!g_target_method || !g_target_method->address()) {
        LOG_ERROR("ExampleHook", "method missing: %s", game::kExampleMethod);
        return false;
    }

    if (!hooks::HookManager::instance().create(
            "Example.TargetMethod",
            g_target_method->address(),
            &hooked_example,
            &g_original)) {
        return false;
    }

    if (!hooks::HookManager::instance().enable("Example.TargetMethod")) {
        hooks::HookManager::instance().remove("Example.TargetMethod");
        return false;
    }

    g_hook_enabled.store(true);
    g_installed.store(true);
    LOG_INFO("ExampleHook", "hooked %s::%s @ %p",
        game::kExampleClass, game::kExampleMethod, g_target_method->address());
    return true;
}

void shutdown() {
    if (!g_installed.load()) {
        return;
    }

    hooks::HookManager::instance().disable("Example.TargetMethod");
    hooks::HookManager::instance().remove("Example.TargetMethod");

    g_target_method = nullptr;
    g_original = nullptr;
    g_hook_enabled.store(false);
    g_installed.store(false);
}

void tick() {}

void draw_hooks_tab() {
    char calls[32]{};
    snprintf(calls, sizeof(calls), "%llu", static_cast<unsigned long long>(g_call_count.load()));

    ui::theme::kv("sample", "example");
    ui::theme::kv("config", game_config_ready() ? "filled" : "blank");
    ui::theme::kv("bound", g_hook_enabled.load() ? "yes" : "no");
    ui::theme::kv("hits", calls);

    ImGui::Dummy(ImVec2(1.0f, 6.0f));
    ui::theme::hairline();
    if (!game_config_ready()) {
        ImGui::TextDisabled("leave kExample* empty to skip, or fill them in config.hpp");
        return;
    }
    ImGui::Text("%s::%s", game::kExampleClass, game::kExampleMethod);
}

} // namespace il2cpp_base::features
