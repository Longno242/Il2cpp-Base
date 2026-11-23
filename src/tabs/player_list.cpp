#include "tabs/tabs.hpp"

#include <imgui.h>

#include <cstdio>

#include "config.hpp"
#include "features/players.hpp"
#include "ui/style.hpp"
#include "ui/widgets.hpp"

namespace il2cpp_base::tabs {

void player_list() {
    const features::players::Snapshot snap = features::players::snapshot();

    ui::section("Player class", [&] {
        static char class_name[128] = {};
        if (!class_name[0]) {
            snprintf(class_name, sizeof(class_name), "%s", config::kPlayerClass);
        }
        ui::input("Class name, e.g. PlayerController or Game.Player", class_name, sizeof(class_name));
        if (ui::button("Apply")) {
            features::players::set_class(class_name);
        }
        ui::value("Tracking", "%s", snap.class_name.c_str());
        ui::value("Status", "%s", snap.class_found ? "found" : "class not found");
    });

    ui::section("Players", [&] {
        if (snap.players.empty()) {
            ui::text("Nobody found.");
            return;
        }
        const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
        if (!ImGui::BeginTable("##players", 4, flags)) {
            return;
        }
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0.4f);
        ImGui::TableSetupColumn("Distance", ImGuiTableColumnFlags_None, 0.15f);
        ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_None, 0.25f);
        ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_None, 0.2f);
        ImGui::TableHeadersRow();

        for (const features::players::Player& p : snap.players) {
            ImGui::PushID(p.object);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(p.name.c_str());
            if (p.local) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ui::style::color::kTextMute);
                ImGui::TextUnformatted("you");
                ImGui::PopStyleColor();
            }
            ImGui::TableNextColumn();
            ImGui::Text("%.0fm", p.distance);
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, ui::style::color::kTextDim);
            ImGui::Text("%.0f, %.0f, %.0f", p.position.x, p.position.y, p.position.z);
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            if (!p.local && ui::button("Teleport")) {
                features::players::teleport_to(p.object);
                ui::notify("Teleported to %s", p.name.c_str());
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    });
}

} // namespace il2cpp_base::tabs
