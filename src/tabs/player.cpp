#include "tabs/tabs.hpp"

#include "features/player.hpp"
#include "features/players.hpp"
#include "ui/widgets.hpp"

namespace il2cpp_base::tabs {

void player() {
    ui::section("Local player", [] {
        const features::players::Snapshot snap = features::players::snapshot();
        const features::players::Player* local = nullptr;
        for (const auto& p : snap.players) {
            if (p.local) {
                local = &p;
            }
        }
        if (!local) {
            ui::text(snap.class_found ? "No %s in the scene." : "Set your player class on the Player List tab.",
                snap.class_name.c_str());
            return;
        }
        ui::value("Name", "%s", local->name.c_str());
        ui::value("Class", "%s", snap.class_name.c_str());
        ui::value("Position", "%.1f  %.1f  %.1f", local->position.x, local->position.y, local->position.z);
        ui::value("Object", "%p", local->object);
    });

    ui::section("Stats", [] {
        field_override("Lock health", features::player::health, 1.0f, 10000.0f);
        field_override("Lock stamina", features::player::stamina, 1.0f, 1000.0f);
    });

    ui::section("Inventory", [] {
        field_override("Lock ammo", features::player::ammo, 1.0f, 999.0f);
        field_override("Lock money", features::player::money, 0.0f, 1000000.0f);
    });
}

} // namespace il2cpp_base::tabs
