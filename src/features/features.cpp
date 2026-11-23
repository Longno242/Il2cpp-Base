#include "features/features.hpp"

#include "features/example_hook.hpp"
#include "features/movement.hpp"
#include "features/player.hpp"
#include "features/players.hpp"
#include "features/visuals.hpp"
#include "features/world.hpp"
#include "unity/unity.hpp"

namespace il2cpp_base::features {

void install() {
    unity::init();
    players::install();
    world::install();
    example_hook::install();
}

void update() {
    players::update();
    player::update();
    movement::update();
    visuals::update();
    world::update();
}

void draw() {
    visuals::draw();
}

void shutdown() {
    example_hook::shutdown();
}

} // namespace il2cpp_base::features
