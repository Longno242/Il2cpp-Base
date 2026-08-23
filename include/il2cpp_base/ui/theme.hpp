#pragma once

#include <imgui.h>

#include <cstdio>

namespace il2cpp_base::ui::theme {

// Survey sheet: forest-ink paper, moss rules, map-cream type.
// Related to Mono's ledger, but a topographic plate — not Matrix green,
// not antivirus, not Windows gray, not cyan cyberpunk.
struct Palette {
    ImVec4 paper     = { 0.11f, 0.13f, 0.12f, 0.97f };
    ImVec4 panel     = { 0.15f, 0.17f, 0.15f, 1.00f };
    ImVec4 inset     = { 0.08f, 0.10f, 0.09f, 1.00f };
    ImVec4 rule      = { 0.30f, 0.34f, 0.28f, 0.90f };
    ImVec4 moss      = { 0.58f, 0.56f, 0.34f, 1.00f };
    ImVec4 moss_dim  = { 0.40f, 0.40f, 0.26f, 1.00f };
    ImVec4 ink       = { 0.86f, 0.87f, 0.78f, 1.00f };
    ImVec4 mute      = { 0.54f, 0.56f, 0.48f, 1.00f };
};

inline const Palette& palette() {
    static const Palette p;
    return p;
}

inline void apply() {
    const Palette& p = palette();
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;

    s.WindowRounding = 2.0f;
    s.ChildRounding = 2.0f;
    s.FrameRounding = 1.0f;
    s.PopupRounding = 2.0f;
    s.ScrollbarRounding = 2.0f;
    s.GrabRounding = 1.0f;
    s.TabRounding = 0.0f;

    s.WindowPadding = ImVec2(16.0f, 14.0f);
    s.FramePadding = ImVec2(8.0f, 5.0f);
    s.ItemSpacing = ImVec2(8.0f, 7.0f);
    s.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    s.ScrollbarSize = 10.0f;
    s.GrabMinSize = 10.0f;
    s.WindowBorderSize = 1.0f;
    s.ChildBorderSize = 1.0f;
    s.FrameBorderSize = 1.0f;
    s.PopupBorderSize = 1.0f;
    s.TabBarBorderSize = 0.0f;

    c[ImGuiCol_Text] = p.ink;
    c[ImGuiCol_TextDisabled] = p.mute;
    c[ImGuiCol_WindowBg] = p.paper;
    c[ImGuiCol_ChildBg] = p.inset;
    c[ImGuiCol_PopupBg] = p.panel;
    c[ImGuiCol_Border] = p.rule;
    c[ImGuiCol_BorderShadow] = { 0, 0, 0, 0 };
    c[ImGuiCol_FrameBg] = p.inset;
    c[ImGuiCol_FrameBgHovered] = { 0.16f, 0.19f, 0.16f, 1.00f };
    c[ImGuiCol_FrameBgActive] = { 0.18f, 0.21f, 0.17f, 1.00f };
    c[ImGuiCol_TitleBg] = p.paper;
    c[ImGuiCol_TitleBgActive] = p.paper;
    c[ImGuiCol_TitleBgCollapsed] = p.paper;
    c[ImGuiCol_MenuBarBg] = p.panel;
    c[ImGuiCol_ScrollbarBg] = p.inset;
    c[ImGuiCol_ScrollbarGrab] = p.moss_dim;
    c[ImGuiCol_ScrollbarGrabHovered] = p.moss;
    c[ImGuiCol_ScrollbarGrabActive] = p.moss;
    c[ImGuiCol_CheckMark] = p.moss;
    c[ImGuiCol_SliderGrab] = p.moss_dim;
    c[ImGuiCol_SliderGrabActive] = p.moss;
    c[ImGuiCol_Button] = p.panel;
    c[ImGuiCol_ButtonHovered] = { 0.22f, 0.24f, 0.18f, 1.00f };
    c[ImGuiCol_ButtonActive] = p.moss_dim;
    c[ImGuiCol_Header] = { 0.20f, 0.22f, 0.16f, 0.85f };
    c[ImGuiCol_HeaderHovered] = { 0.24f, 0.26f, 0.18f, 1.00f };
    c[ImGuiCol_HeaderActive] = p.moss_dim;
    c[ImGuiCol_Separator] = p.rule;
    c[ImGuiCol_SeparatorHovered] = p.moss_dim;
    c[ImGuiCol_SeparatorActive] = p.moss;
    c[ImGuiCol_ResizeGrip] = p.rule;
    c[ImGuiCol_ResizeGripHovered] = p.moss_dim;
    c[ImGuiCol_ResizeGripActive] = p.moss;
    c[ImGuiCol_Tab] = p.paper;
    c[ImGuiCol_TabHovered] = p.panel;
    c[ImGuiCol_TabSelected] = p.panel;
    c[ImGuiCol_TabSelectedOverline] = p.moss;
    c[ImGuiCol_TabDimmed] = p.paper;
    c[ImGuiCol_TabDimmedSelected] = p.panel;
    c[ImGuiCol_TabDimmedSelectedOverline] = p.moss_dim;
}

inline void hairline() {
    const Palette& p = palette();
    ImVec2 a = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddLine(
        a, ImVec2(a.x + w, a.y), ImGui::ColorConvertFloat4ToU32(p.rule), 1.0f);
    ImGui::Dummy(ImVec2(1.0f, 8.0f));
}

inline void accent_rule() {
    const Palette& p = palette();
    ImVec2 a = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddLine(
        a, ImVec2(a.x + w, a.y), ImGui::ColorConvertFloat4ToU32(p.moss), 2.0f);
    ImGui::Dummy(ImVec2(1.0f, 10.0f));
}

inline void header(const char* title, const char* kicker, const char* meta) {
    const Palette& p = palette();
    ImGui::PushStyleColor(ImGuiCol_Text, p.mute);
    ImGui::TextUnformatted(kicker);
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, p.ink);
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    if (meta && *meta) {
        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(meta).x - 20.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, p.mute);
        ImGui::TextUnformatted(meta);
        ImGui::PopStyleColor();
    }
    accent_rule();
}

inline void kv(const char* key, const char* value) {
    const Palette& p = palette();
    ImGui::PushStyleColor(ImGuiCol_Text, p.mute);
    ImGui::Text("%-10s", key);
    ImGui::PopStyleColor();
    ImGui::SameLine(96.0f);
    ImGui::TextUnformatted(value ? value : "—");
}

inline bool index_item(const char* label, bool selected) {
    const Palette& p = palette();
    ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0, 0, 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0, 0, 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_Text, selected ? p.moss : p.mute);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
    const bool pressed = ImGui::SmallButton(label);
    ImGui::PopStyleVar();
    if (selected) {
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(min.x, max.y + 1.0f),
            ImVec2(max.x, max.y + 1.0f),
            ImGui::ColorConvertFloat4ToU32(p.moss),
            2.0f);
    }
    ImGui::PopStyleColor(4);
    return pressed;
}

inline int index_bar(const char* const* labels, int count, int current) {
    int next = current;
    for (int i = 0; i < count; ++i) {
        if (i > 0) {
            ImGui::SameLine(0.0f, 18.0f);
        }
        if (index_item(labels[i], current == i)) {
            next = i;
        }
    }
    ImGui::Dummy(ImVec2(1.0f, 10.0f));
    hairline();
    return next;
}

inline void begin_inset(const char* id, float height) {
    ImGui::BeginChild(id, ImVec2(0.0f, height), ImGuiChildFlags_Borders);
}

inline void end_inset() {
    ImGui::EndChild();
}

inline void footer(const char* line) {
    hairline();
    ImGui::PushStyleColor(ImGuiCol_Text, palette().mute);
    ImGui::TextUnformatted(line);
    ImGui::PopStyleColor();
}

} // namespace il2cpp_base::ui::theme
