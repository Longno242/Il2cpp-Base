#include "tabs/tabs.hpp"

#include "config.hpp"
#include "il2cpp/resolver.hpp"
#include "ui/overlay.hpp"
#include "ui/style.hpp"
#include "ui/widgets.hpp"

namespace il2cpp_base::tabs {

void settings() {
    ui::Options& options = ui::options();

    ui::section("Interface", [&] {
        float scale = ui::style::scale();
        if (ui::slider("UI scale", &scale, 0.75f, 2.0f, "%.2f")) {
            ui::style::set_scale(scale);
        }
        ui::toggle("Watermark", &options.watermark);
        ui::toggle("Software cursor", &options.software_cursor);
    });

    ui::section("Input", [&] {
        ui::toggle("Block game input over menu", &options.block_input);
        ui::toggle("Unlock cursor while open", &options.unlock_cursor);
    });

    ui::section("About", [] {
        ui::value("Version", "%s", IL2CPP_BASE_VERSION);
        ui::value("Menu key", "%s", ui::overlay::menu_key_name());
        ui::value("IL2CPP", "%s", il2cpp::ready() ? "ready" : "waiting");
        ui::value("Log file", "%s", config::kLogFileName);
    });
}

} // namespace il2cpp_base::tabs
