#include "features/example_hook.hpp"

#include <atomic>

#include "core/hooks.hpp"
#include "core/logger.hpp"
#include "il2cpp/resolver.hpp"

namespace il2cpp_base::features::example_hook {
namespace {

constexpr const char* kAssembly = "Assembly-CSharp.dll";
constexpr const char* kNamespace = "";
constexpr const char* kClass = "PlayerController";
constexpr const char* kMethod = "Update";
constexpr const char* kHookName = "Example.PlayerController.Update";

// IL2CPP instance methods take `this` first and the MethodInfo* last.
using update_fn = void (*)(void* self, const void* method);
update_fn original = nullptr;

std::atomic<unsigned long long> call_count{ 0 };
std::atomic<void*> instance{ nullptr };

void hk_update(void* self, const void* method) {
    call_count.fetch_add(1, std::memory_order_relaxed);
    instance.store(self, std::memory_order_relaxed);

    // Change state before or after the original runs, or skip it entirely.
    original(self, method);
}

} // namespace

void install() {
    il2cpp::Image* image = il2cpp::image(kAssembly);
    il2cpp::Class* klass = image ? image->klass(kClass, kNamespace) : nullptr;
    il2cpp::Method* method = klass ? klass->method(kMethod, 0) : nullptr;
    if (!method || !method->address()) {
        LOG_INFO("Example", "%s.%s not in this game, example hook skipped", kClass, kMethod);
        return;
    }

    if (hooks::install(kHookName, method->address(), &hk_update, &original)) {
        hooks::set_enabled(kHookName, enabled);
    }
}

void shutdown() {
    hooks::remove(kHookName);
}

void set_enabled(bool on) {
    enabled = on;
    hooks::set_enabled(kHookName, on);
}

bool installed() {
    return original != nullptr;
}

const char* target() {
    return kHookName;
}

unsigned long long calls() {
    return call_count.load(std::memory_order_relaxed);
}

void* last_instance() {
    return instance.load(std::memory_order_relaxed);
}

} // namespace il2cpp_base::features::example_hook
