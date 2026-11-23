#include "features/players.hpp"

#include <Windows.h>

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <mutex>

#include "config.hpp"
#include "core/logger.hpp"
#include "core/main_thread.hpp"
#include "il2cpp/helpers.hpp"

namespace il2cpp_base::features::players {
namespace {

constexpr ULONGLONG kRefreshMs = 500;
constexpr std::size_t kMaxPlayers = 256;

struct Tracked {
    std::uint32_t object_handle = 0;
    std::uint32_t transform_handle = 0;
    void* object = nullptr;
    void* transform = nullptr;
    std::string name;
};

// Main thread state.
il2cpp::Class* klass = nullptr;
std::string class_name = config::kPlayerClass;
bool class_dirty = true;
std::vector<Tracked> tracked;
ULONGLONG last_refresh = 0;
std::size_t cached_ptr_offset = 0;
void* local = nullptr;
void* local_tr = nullptr;

// Shared with the render thread.
std::mutex mutex;
Snapshot shared;

il2cpp::Class* resolve_class(const std::string& full_name) {
    const std::size_t dot = full_name.rfind('.');
    if (dot != std::string::npos) {
        return il2cpp::search_class(full_name.substr(dot + 1), full_name.substr(0, dot));
    }
    if (il2cpp::Class* found = il2cpp::search_class(full_name)) {
        return found;
    }
    for (il2cpp::Image* image : il2cpp::images()) {
        for (il2cpp::Class* candidate : image->classes()) {
            if (candidate->name() == full_name) {
                return candidate;
            }
        }
    }
    return nullptr;
}

// UnityEngine.Object.m_CachedPtr is cleared when the native object is destroyed.
bool alive(void* object) {
    if (!object) {
        return false;
    }
    if (!cached_ptr_offset) {
        return true;
    }
    void* native = nullptr;
    return il2cpp::mem_read(static_cast<std::uint8_t*>(object) + cached_ptr_offset, native) && native;
}

void release_all() {
    for (const Tracked& t : tracked) {
        il2cpp::gc_release(t.object_handle);
        il2cpp::gc_release(t.transform_handle);
    }
    tracked.clear();
    local = nullptr;
    local_tr = nullptr;
}

void refresh() {
    release_all();
    if (!klass) {
        return;
    }
    for (void* object : unity::find_objects(klass)) {
        if (tracked.size() >= kMaxPlayers) {
            break;
        }
        if (!alive(object)) {
            continue;
        }
        void* transform = unity::transform(object);
        if (!transform) {
            continue;
        }
        tracked.push_back({ il2cpp::gc_hold(object), il2cpp::gc_hold(transform), object, transform, unity::name(object) });
    }
}

} // namespace

void install() {
    if (il2cpp::Class* object_class = il2cpp::search_class("Object", "UnityEngine")) {
        if (il2cpp::Field* field = object_class->field("m_CachedPtr")) {
            cached_ptr_offset = field->offset();
        }
    }
}

void update() {
    if (class_dirty) {
        class_dirty = false;
        release_all();
        klass = resolve_class(class_name);
        LOG_INFO("Players", klass ? "tracking %s" : "class %s not found", class_name.c_str());
        last_refresh = 0;
    }

    const ULONGLONG now = GetTickCount64();
    if (now - last_refresh >= kRefreshMs) {
        last_refresh = now;
        refresh();
    }

    void* camera = unity::main_camera();
    const unity::Vector3 camera_pos = unity::position(unity::transform(camera));
    const float sw = static_cast<float>(std::max(1, unity::screen_width()));
    const float sh = static_cast<float>(std::max(1, unity::screen_height()));

    Snapshot next;
    next.class_name = class_name;
    next.class_found = klass != nullptr;
    next.players.reserve(tracked.size());

    float closest = FLT_MAX;
    std::size_t local_index = SIZE_MAX;
    for (const Tracked& t : tracked) {
        if (!alive(t.object) || !alive(t.transform)) {
            continue;
        }
        Player p;
        p.object = t.object;
        p.name = t.name;
        p.position = unity::position(t.transform);
        p.distance = unity::distance(p.position, camera_pos);

        unity::Vector3 screen;
        if (unity::world_to_screen(camera, p.position, screen)) {
            p.on_screen = true;
            p.screen_x = screen.x / sw;
            p.screen_y = 1.0f - screen.y / sh;
        }
        if (p.distance < closest) {
            closest = p.distance;
            local_index = next.players.size();
        }
        next.players.push_back(std::move(p));
    }

    local = nullptr;
    local_tr = nullptr;
    if (local_index != SIZE_MAX) {
        next.players[local_index].local = true;
        local = next.players[local_index].object;
        for (const Tracked& t : tracked) {
            if (t.object == local) {
                local_tr = t.transform;
            }
        }
    }
    std::sort(next.players.begin(), next.players.end(), [](const Player& a, const Player& b) {
        return a.distance < b.distance;
    });

    std::lock_guard lock(mutex);
    shared = std::move(next);
}

void* local_player() {
    return local;
}

void* local_transform() {
    return local_tr;
}

il2cpp::Class* player_class() {
    return klass;
}

Snapshot snapshot() {
    std::lock_guard lock(mutex);
    return shared;
}

void set_class(const std::string& name) {
    main_thread::post([name] {
        class_name = name;
        class_dirty = true;
    });
}

void teleport_to(void* target) {
    main_thread::post([target] {
        for (const Tracked& t : tracked) {
            if (t.object == target && alive(t.transform) && local_tr && t.object != local) {
                unity::set_position(local_tr, unity::position(t.transform));
                return;
            }
        }
    });
}

} // namespace il2cpp_base::features::players
