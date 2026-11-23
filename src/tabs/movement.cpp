#include "tabs/tabs.hpp"

#include "features/movement.hpp"
#include "ui/widgets.hpp"

namespace il2cpp_base::tabs {

void movement() {
    namespace mv = features::movement;

    ui::section("Speed", [] {
        ui::toggle("Speed hack", &mv::speed_enabled);
        ui::slider("Multiplier", &mv::speed, 0.1f, 10.0f, "%.1fx");
        ui::text("Changes Time.timeScale, so everything speeds up.");
    });

    ui::section("Gravity", [] {
        ui::toggle("Custom gravity", &mv::gravity_enabled);
        ui::slider("Gravity", &mv::gravity, -30.0f, 10.0f, "%.1f");
        ui::text("Unity's default is -9.8.");
    });

    ui::section("Player fields", [] {
        field_override("Move speed", mv::move_speed, 0.0f, 100.0f, "%.1f");
        field_override("Jump force", mv::jump_force, 0.0f, 100.0f, "%.1f");
    });
}

} // namespace il2cpp_base::tabs
