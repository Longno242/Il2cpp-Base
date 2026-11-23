#include "ui/cursor.hpp"

#include "core/hooks.hpp"
#include "core/logger.hpp"
#include "il2cpp/resolver.hpp"
#include "ui/overlay.hpp"

namespace il2cpp_base::ui::cursor {
namespace {

constexpr int kLockNone = 0;

using set_lock_fn = void (*)(int, const void*);
using get_lock_fn = int (*)(const void*);
using set_visible_fn = void (*)(bool, const void*);
using get_visible_fn = bool (*)(const void*);

struct Accessors {
    set_lock_fn set_lock = nullptr;
    get_lock_fn get_lock = nullptr;
    set_visible_fn set_visible = nullptr;
    get_visible_fn get_visible = nullptr;
    const void* set_lock_info = nullptr;
    const void* get_lock_info = nullptr;
    const void* set_visible_info = nullptr;
    const void* get_visible_info = nullptr;
};

Accessors api;
int game_lock = kLockNone;
bool game_visible = true;
bool forcing = false;

bool active() {
    return overlay::menu_open() && options().unlock_cursor;
}

void hk_set_lock(int state, const void* method) {
    game_lock = state;
    if (!forcing) {
        api.set_lock(state, method);
    }
}

void hk_set_visible(bool visible, const void* method) {
    game_visible = visible;
    if (!forcing) {
        api.set_visible(visible, method);
    }
}

il2cpp::Class* find_cursor_class() {
    for (const char* module : { "UnityEngine.CoreModule.dll", "UnityEngine.dll" }) {
        if (il2cpp::Image* image = il2cpp::image(module)) {
            if (il2cpp::Class* klass = image->klass("Cursor", "UnityEngine")) {
                return klass;
            }
        }
    }
    return nullptr;
}

template <typename Fn>
bool resolve(il2cpp::Class* klass, const char* name, int args, Fn& fn, const void*& info) {
    il2cpp::Method* method = klass->method(name, args);
    if (!method || !method->address()) {
        return false;
    }
    fn = reinterpret_cast<Fn>(method->address());
    info = method->raw();
    return true;
}

} // namespace

void install() {
    il2cpp::Class* klass = find_cursor_class();
    if (!klass) {
        LOG_WARN("Cursor", "UnityEngine.Cursor not found, cursor unlock disabled");
        return;
    }

    Accessors found;
    if (!resolve(klass, "set_lockState", 1, found.set_lock, found.set_lock_info) ||
        !resolve(klass, "get_lockState", 0, found.get_lock, found.get_lock_info) ||
        !resolve(klass, "set_visible", 1, found.set_visible, found.set_visible_info) ||
        !resolve(klass, "get_visible", 0, found.get_visible, found.get_visible_info)) {
        LOG_WARN("Cursor", "Cursor accessors not found, cursor unlock disabled");
        return;
    }

    api = found;
    game_lock = api.get_lock(api.get_lock_info);
    game_visible = api.get_visible(api.get_visible_info);

    const bool ok = hooks::install("Cursor.set_lockState", reinterpret_cast<void*>(found.set_lock),
                        &hk_set_lock, &api.set_lock) &&
                    hooks::install("Cursor.set_visible", reinterpret_cast<void*>(found.set_visible),
                        &hk_set_visible, &api.set_visible);
    if (!ok) {
        hooks::remove("Cursor.set_lockState");
        api = {};
    }
}

void update() {
    if (!api.set_lock) {
        return;
    }

    if (active()) {
        forcing = true;
        if (api.get_lock(api.get_lock_info) != kLockNone) {
            api.set_lock(kLockNone, api.set_lock_info);
        }
        if (!api.get_visible(api.get_visible_info)) {
            api.set_visible(true, api.set_visible_info);
        }
    } else if (forcing) {
        forcing = false;
        api.set_lock(game_lock, api.set_lock_info);
        api.set_visible(game_visible, api.set_visible_info);
    }
}

} // namespace il2cpp_base::ui::cursor
