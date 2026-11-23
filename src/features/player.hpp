#pragma once

#include "features/field_override.hpp"

// Local player tweaks. The field names are guesses that cover a lot of games; change them in the
// menu or here once you've found the real ones in the Explorer tab.
namespace il2cpp_base::features::player {

inline FieldOverride health{ "health", 100.0f };
inline FieldOverride stamina{ "stamina", 100.0f };
inline FieldOverride ammo{ "currentAmmo", 30.0f };
inline FieldOverride money{ "money", 99999.0f };

void update();

} // namespace il2cpp_base::features::player
