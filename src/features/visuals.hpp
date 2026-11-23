#pragma once

namespace il2cpp_base::features::visuals {

inline bool esp = false;
inline bool esp_names = true;
inline bool esp_distance = true;
inline bool esp_lines = false;
inline bool esp_show_local = false;
inline float esp_max_distance = 300.0f;
inline float esp_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

inline bool fov_enabled = false;
inline float fov = 90.0f;

inline bool crosshair = false;
inline float crosshair_size = 6.0f;
inline float crosshair_color[4] = { 1.0f, 1.0f, 1.0f, 0.9f };

void update(); // main thread
void draw();   // render thread

} // namespace il2cpp_base::features::visuals
