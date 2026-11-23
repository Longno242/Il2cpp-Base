#pragma once

#include <string>

namespace il2cpp_base::features::world {

inline bool no_fog = false;
inline bool fps_unlock = false;
inline int fps_target = 240;
inline bool vsync_off = false;

struct Info {
    std::string product;
    std::string unity_version;
};

void install(); // main thread
void update();  // main thread
Info info();    // any thread

} // namespace il2cpp_base::features::world
