#include "tabs/tabs.hpp"

#include "features/world.hpp"
#include "ui/widgets.hpp"

namespace il2cpp_base::tabs {

void world() {
    namespace w = features::world;

    ui::section("Rendering", [] {
        ui::toggle("No fog", &w::no_fog);
    });

    ui::section("Performance", [] {
        ui::toggle("Unlock FPS", &w::fps_unlock);
        ui::slider("Target FPS", &w::fps_target, 30, 500);
        ui::toggle("Disable VSync", &w::vsync_off);
    });

    ui::section("Game", [] {
        const w::Info info = w::info();
        ui::value("Product", "%s", info.product.empty() ? "-" : info.product.c_str());
        ui::value("Unity", "%s", info.unity_version.empty() ? "-" : info.unity_version.c_str());
    });
}

} // namespace il2cpp_base::tabs
