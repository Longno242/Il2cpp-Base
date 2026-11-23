#include "tabs/tabs.hpp"

#include <imgui.h>

#include "features/example_hook.hpp"
#include "features/movement.hpp"
#include "features/player.hpp"
#include "features/players.hpp"
#include "features/visuals.hpp"
#include "features/world.hpp"
#include "il2cpp/resolver.hpp"
#include "ui/overlay.hpp"
#include "ui/widgets.hpp"

namespace il2cpp_base::tabs {

void home() {
    ui::section("Status", [] {
        const features::world::Info info = features::world::info();
        const features::players::Snapshot players = features::players::snapshot();
        ui::value("Game", "%s", info.product.empty() ? "-" : info.product.c_str());
        ui::value("Unity", "%s", info.unity_version.empty() ? "-" : info.unity_version.c_str());
        ui::value("IL2CPP", "%s", il2cpp::ready() ? "ready" : "waiting");
        ui::value("Players", "%zu", players.players.size());
        ui::value("FPS", "%.0f", ImGui::GetIO().Framerate);
    });

    ui::section("Quick toggles", [] {
        ui::toggle("ESP", &features::visuals::esp);
        ui::toggle("Speed hack", &features::movement::speed_enabled);
        ui::toggle("Lock health", &features::player::health.enabled);
        ui::toggle("No fog", &features::world::no_fog);
        ui::toggle("Crosshair", &features::visuals::crosshair);
    });

    ui::section("Getting started", [] {
        ui::text("1. Find your game's player class in the Explorer tab.");
        ui::text("2. Enter it on the Player List tab and hit Apply.");
        ui::text("3. Player, Movement and Visuals then work off that class.");
        ui::text("Press %s to hide this menu.", ui::overlay::menu_key_name());
    });

    ui::section("Hook example", [] {
        namespace hook = features::example_hook;
        if (!hook::installed()) {
            ui::text("Point features/example_hook.cpp at a method in your game.");
            return;
        }
        if (ui::toggle("Enabled", &hook::enabled)) {
            hook::set_enabled(hook::enabled);
        }
        ui::value("Target", "%s", hook::target());
        ui::value("Calls", "%llu", hook::calls());
    });
}

} // namespace il2cpp_base::tabs
