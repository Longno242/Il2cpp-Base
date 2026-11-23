#include "features/world.hpp"

#include <mutex>

#include "features/restore.hpp"
#include "unity/unity.hpp"

namespace il2cpp_base::features::world {
namespace {

Restorable<bool> fog;
Restorable<int> frame_rate;
Restorable<int> vsync;

std::mutex info_mutex;
Info game_info;

} // namespace

void install() {
    Info next{ unity::product_name(), unity::unity_version() };
    std::lock_guard lock(info_mutex);
    game_info = std::move(next);
}

void update() {
    fog.apply(no_fog, false, unity::fog, unity::set_fog);
    frame_rate.apply(fps_unlock, fps_target, unity::target_frame_rate, unity::set_target_frame_rate);
    vsync.apply(vsync_off || fps_unlock, 0, unity::vsync_count, unity::set_vsync_count);
}

Info info() {
    std::lock_guard lock(info_mutex);
    return game_info;
}

} // namespace il2cpp_base::features::world
