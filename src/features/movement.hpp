#pragma once

#include "features/field_override.hpp"

namespace il2cpp_base::features::movement {

// Speed hack through Time.timeScale. Speeds up the whole game, not just the player.
inline bool speed_enabled = false;
inline float speed = 2.0f;

inline bool gravity_enabled = false;
inline float gravity = -4.0f;

inline FieldOverride move_speed{ "moveSpeed", 12.0f };
inline FieldOverride jump_force{ "jumpForce", 12.0f };

void update();

} // namespace il2cpp_base::features::movement
