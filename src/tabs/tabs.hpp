#pragma once

namespace il2cpp_base::features {
struct FieldOverride;
}

// One function per sidebar tab. They run on the render thread: read and write feature settings
// freely, but queue anything that calls into the game with main_thread::post.
namespace il2cpp_base::tabs {

void home();
void player();
void movement();
void player_list();
void visuals();
void world();
void settings();

// Toggle, value slider and editable field name for a FieldOverride.
void field_override(const char* label, features::FieldOverride& field, float min, float max, const char* format = "%.0f");

} // namespace il2cpp_base::tabs
