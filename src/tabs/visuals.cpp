#include "tabs/tabs.hpp"

#include "features/visuals.hpp"
#include "ui/widgets.hpp"

namespace il2cpp_base::tabs {

void visuals() {
    namespace vis = features::visuals;

    ui::section("ESP", [] {
        ui::toggle("Enabled", &vis::esp);
        ui::toggle("Names", &vis::esp_names);
        ui::toggle("Distance", &vis::esp_distance);
        ui::toggle("Snaplines", &vis::esp_lines);
        ui::toggle("Show yourself", &vis::esp_show_local);
        ui::slider("Max distance", &vis::esp_max_distance, 10.0f, 1000.0f, "%.0fm");
        ui::color("Color", vis::esp_color);
    });

    ui::section("Camera", [] {
        ui::toggle("Custom FOV", &vis::fov_enabled);
        ui::slider("Field of view", &vis::fov, 30.0f, 150.0f, "%.0f");
    });

    ui::section("Crosshair", [] {
        ui::toggle("Enabled", &vis::crosshair);
        ui::slider("Size", &vis::crosshair_size, 2.0f, 20.0f, "%.0f");
        ui::color("Color", vis::crosshair_color);
    });
}

} // namespace il2cpp_base::tabs
