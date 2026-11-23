#pragma once

// Every feature gets wired in here once. install/update/shutdown run on the game's main thread,
// draw runs on the render thread every frame (menu open or not) and must not call into IL2CPP.
namespace il2cpp_base::features {

void install();
void update();
void draw();
void shutdown();

} // namespace il2cpp_base::features
