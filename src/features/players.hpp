#pragma once

#include <string>
#include <vector>

#include "il2cpp/types.hpp"
#include "unity/unity.hpp"

// Tracks every live object of the player class (config::kPlayerClass, changeable from the menu).
// The one closest to the main camera is treated as the local player.
namespace il2cpp_base::features::players {

struct Player {
    void* object = nullptr;
    std::string name;
    unity::Vector3 position;
    float distance = 0.0f;
    bool local = false;
    bool on_screen = false;
    float screen_x = 0.0f; // 0..1 from the left
    float screen_y = 0.0f; // 0..1 from the top
};

struct Snapshot {
    std::vector<Player> players;
    std::string class_name;
    bool class_found = false;
};

// Main thread.
void install();
void update();
void* local_player();
void* local_transform();
il2cpp::Class* player_class();

// Any thread.
Snapshot snapshot();
void set_class(const std::string& name);
void teleport_to(void* target);

} // namespace il2cpp_base::features::players
