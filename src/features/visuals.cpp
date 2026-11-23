#include "features/visuals.hpp"

#include <imgui.h>

#include <cstdio>

#include "features/players.hpp"
#include "features/restore.hpp"
#include "ui/style.hpp"
#include "unity/unity.hpp"

namespace il2cpp_base::features::visuals {
namespace {

Restorable<float> camera_fov;
void* fov_camera = nullptr;

void draw_esp() {
    const players::Snapshot snap = players::snapshot();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(esp_color[0], esp_color[1], esp_color[2], esp_color[3]));
    const ImU32 shadow = IM_COL32(0, 0, 0, 180);
    const float r = ui::style::px(3.0f);

    for (const players::Player& p : snap.players) {
        if (!p.on_screen || p.distance > esp_max_distance || (p.local && !esp_show_local)) {
            continue;
        }
        const ImVec2 at(p.screen_x * display.x, p.screen_y * display.y);

        if (esp_lines) {
            dl->AddLine(ImVec2(display.x * 0.5f, display.y), at, color, 1.0f);
        }
        dl->AddCircleFilled(at, r + 1.0f, shadow);
        dl->AddCircleFilled(at, r, color);

        char label[160]{};
        if (esp_names && esp_distance) {
            snprintf(label, sizeof(label), "%s  %.0fm", p.name.c_str(), p.distance);
        } else if (esp_names) {
            snprintf(label, sizeof(label), "%s", p.name.c_str());
        } else if (esp_distance) {
            snprintf(label, sizeof(label), "%.0fm", p.distance);
        }
        if (label[0]) {
            const ImVec2 size = ImGui::CalcTextSize(label);
            const ImVec2 pos(at.x - size.x * 0.5f, at.y - size.y - r - ui::style::px(4.0f));
            dl->AddText(ImVec2(pos.x + 1.0f, pos.y + 1.0f), shadow, label);
            dl->AddText(pos, color, label);
        }
    }
}

void draw_crosshair() {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 c(display.x * 0.5f, display.y * 0.5f);
    const float s = ui::style::px(crosshair_size);
    const float gap = ui::style::px(3.0f);
    const ImU32 color = ImGui::ColorConvertFloat4ToU32(
        ImVec4(crosshair_color[0], crosshair_color[1], crosshair_color[2], crosshair_color[3]));
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImVec2 lines[4][2] = {
        { ImVec2(c.x - gap - s, c.y), ImVec2(c.x - gap, c.y) },
        { ImVec2(c.x + gap, c.y), ImVec2(c.x + gap + s, c.y) },
        { ImVec2(c.x, c.y - gap - s), ImVec2(c.x, c.y - gap) },
        { ImVec2(c.x, c.y + gap), ImVec2(c.x, c.y + gap + s) },
    };
    for (const auto& line : lines) {
        dl->AddLine(line[0], line[1], IM_COL32(0, 0, 0, 160), 3.0f);
        dl->AddLine(line[0], line[1], color, 1.5f);
    }
}

} // namespace

void update() {
    void* camera = unity::main_camera();
    if (camera != fov_camera) {
        // New scene or camera; the old one's FOV can't be restored.
        fov_camera = camera;
        camera_fov = {};
    }
    camera_fov.apply(fov_enabled && camera, fov,
        [&] { return unity::field_of_view(camera); },
        [&](float value) { unity::set_field_of_view(camera, value); });
}

void draw() {
    if (esp) {
        draw_esp();
    }
    if (crosshair) {
        draw_crosshair();
    }
}

} // namespace il2cpp_base::features::visuals
