#include "features/movement.hpp"

#include "features/players.hpp"
#include "features/restore.hpp"
#include "unity/unity.hpp"

namespace il2cpp_base::features::movement {
namespace {

Restorable<float> time_scale;
Restorable<unity::Vector3> world_gravity;

} // namespace

void update() {
    time_scale.apply(speed_enabled, speed, unity::time_scale, unity::set_time_scale);
    world_gravity.apply(gravity_enabled, unity::Vector3{ 0.0f, gravity, 0.0f }, unity::gravity, unity::set_gravity);

    void* local = players::local_player();
    il2cpp::Class* klass = players::player_class();
    move_speed.apply(local, klass);
    jump_force.apply(local, klass);
}

} // namespace il2cpp_base::features::movement
