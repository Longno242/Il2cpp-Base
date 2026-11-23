#include "ui/widgets.hpp"

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <deque>
#include <mutex>

#include "il2cpp/resolver.hpp"
#include "ui/overlay.hpp"
#include "ui/style.hpp"

#ifndef IL2CPP_BASE_VERSION
#define IL2CPP_BASE_VERSION "dev"
#endif

namespace il2cpp_base::ui {
namespace {

using style::px;
namespace color = style::color;

struct Page {
    ImVec2 origin;
    float width = 0.0f;
    float column_y[2]{};
    int columns = 1;
    int count = 0;
    int column = 0;
};

Page g_page;
int g_tab = 0;
int g_shown_tab = -1;
float g_tab_fade = 0.0f;

struct Toast {
    std::string text;
    double start = -1.0;
};

std::mutex g_toast_mutex;
std::deque<Toast> g_toasts;

ImU32 col(ImU32 c, float alpha = 1.0f) {
    return ImGui::GetColorU32(c, alpha);
}

ImU32 mix(ImU32 a, ImU32 b, float t) {
    return ImGui::GetColorU32(ImLerp(ImGui::ColorConvertU32ToFloat4(a), ImGui::ColorConvertU32ToFloat4(b), t));
}

ImU32 with_alpha(ImU32 c, float alpha) {
    const auto a = static_cast<ImU32>(ImSaturate(alpha) * ((c >> IM_COL32_A_SHIFT) & 0xFF));
    return (c & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

float animate(ImGuiID id, float target, float speed = 16.0f) {
    float* value = ImGui::GetStateStorage()->GetFloatRef(ImHashStr("##anim", 0, id), target);
    *value += (target - *value) * (1.0f - std::exp(-speed * ImGui::GetIO().DeltaTime));
    if (std::fabs(target - *value) < 0.002f) {
        *value = target;
    }
    return *value;
}

float ease_out(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

void label_text(ImVec2 pos, float row_height, const char* label, ImU32 c) {
    const char* end = ImGui::FindRenderedTextEnd(label);
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(pos.x, pos.y + (row_height - ImGui::GetFontSize()) * 0.5f), c, label, end);
}

bool sidebar_item(const char* label, bool selected, float width) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(label);
    const ImVec2 pos = window->DC.CursorPos;
    const ImVec2 size(width, px(34.0f));
    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id)) {
        return false;
    }

    bool hovered = false;
    bool held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const float sel = animate(id, selected ? 1.0f : 0.0f);
    const float hov = animate(id + 1, hovered ? 1.0f : 0.0f);

    ImDrawList* dl = window->DrawList;
    const float bg = std::max(sel, hov * 0.55f);
    if (bg > 0.0f) {
        dl->AddRectFilled(bb.Min, bb.Max, col(color::kControl, bg), px(7.0f));
    }
    if (sel > 0.0f) {
        const float bar = size.y * 0.42f * sel;
        const float cy = bb.GetCenter().y;
        dl->AddRectFilled(ImVec2(bb.Min.x, cy - bar * 0.5f), ImVec2(bb.Min.x + px(3.0f), cy + bar * 0.5f),
            col(color::kAccent, sel), px(2.0f));
    }
    label_text(ImVec2(bb.Min.x + px(16.0f), bb.Min.y), size.y, label,
        mix(color::kTextDim, color::kText, std::max(sel, hov * 0.6f)));
    return pressed;
}

void draw_sidebar_footer(ImVec2 pos, float sidebar_w, float height) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float font = ImGui::GetFontSize();
    const ImVec2 base(pos.x + px(20.0f), pos.y + height - px(54.0f));
    const bool ready = il2cpp::ready();

    dl->AddLine(ImVec2(pos.x + px(14.0f), base.y - px(14.0f)),
        ImVec2(pos.x + sidebar_w - px(14.0f), base.y - px(14.0f)), col(color::kBorder));
    dl->AddCircleFilled(ImVec2(base.x + px(3.0f), base.y + font * 0.5f), px(3.5f),
        col(ready ? color::kAccent : color::kTextMute));
    dl->AddText(ImVec2(base.x + px(14.0f), base.y), col(color::kTextDim),
        ready ? "Runtime ready" : "Waiting for runtime");

    char hint[64]{};
    snprintf(hint, sizeof(hint), "%s to hide", overlay::menu_key_name());
    dl->AddText(ImVec2(base.x, base.y + font + px(6.0f)), col(color::kTextMute), hint);
}

bool slider_core(const char* label, float* v, float min, float max, bool integer, const char* format) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    const ImGuiID id = window->GetID(label);
    const float w = ImGui::GetContentRegionAvail().x;
    const float font = ImGui::GetFontSize();
    const float hit_h = px(18.0f);
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, pos + ImVec2(w, font + px(4.0f) + hit_h));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) {
        return false;
    }

    const ImRect hit(ImVec2(bb.Min.x, bb.Max.y - hit_h), bb.Max);
    const float knob_r = px(6.0f);
    const float x0 = hit.Min.x + knob_r;
    const float x1 = hit.Max.x - knob_r;

    bool hovered = false;
    bool held = false;
    ImGui::ButtonBehavior(hit, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

    bool changed = false;
    if (held && max > min && x1 > x0) {
        const float t = ImSaturate((ImGui::GetIO().MousePos.x - x0) / (x1 - x0));
        float next = min + (max - min) * t;
        if (integer) {
            next = std::round(next);
        }
        if (next != *v) {
            *v = next;
            changed = true;
            ImGui::MarkItemEdited(id);
        }
    }

    char value_text[64]{};
    if (integer) {
        snprintf(value_text, sizeof(value_text), format, static_cast<int>(*v));
    } else {
        snprintf(value_text, sizeof(value_text), format, *v);
    }

    ImDrawList* dl = window->DrawList;
    label_text(bb.Min, font, label, col(color::kText));
    const float value_w = ImGui::CalcTextSize(value_text).x;
    dl->AddText(ImVec2(bb.Max.x - value_w, bb.Min.y), col(color::kTextDim), value_text);

    const float t = max > min ? ImSaturate((*v - min) / (max - min)) : 0.0f;
    const float kx = ImLerp(x0, x1, t);
    const float cy = hit.GetCenter().y;
    const float track = px(4.0f);
    dl->AddRectFilled(ImVec2(hit.Min.x, cy - track * 0.5f), ImVec2(hit.Max.x, cy + track * 0.5f),
        col(color::kControlPress), track);
    dl->AddRectFilled(ImVec2(hit.Min.x, cy - track * 0.5f), ImVec2(kx, cy + track * 0.5f),
        col(color::kAccent), track);

    const float grow = animate(id, (hovered || held) ? 1.0f : 0.0f);
    dl->AddCircleFilled(ImVec2(kx, cy), knob_r + px(1.5f) * grow, col(color::kAccent));
    dl->AddCircleFilled(ImVec2(kx, cy), px(2.0f) * (1.0f - grow), col(color::kOnAccent));
    return changed;
}

} // namespace

void window(const char* title, std::span<const Tab> tabs) {
    if (tabs.empty()) {
        return;
    }
    g_tab = std::clamp(g_tab, 0, static_cast<int>(tabs.size()) - 1);

    const ImVec2 size(px(820.0f), px(520.0f));
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    const bool visible = ImGui::Begin("##il2cpp_base_menu", nullptr, flags);
    ImGui::PopStyleVar();
    if (!visible) {
        ImGui::End();
        return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetWindowPos();
    const float sidebar_w = px(190.0f);
    dl->AddRectFilled(pos, pos + ImVec2(sidebar_w, size.y), col(color::kSidebar),
        ImGui::GetStyle().WindowRounding, ImDrawFlags_RoundCornersLeft);
    dl->AddLine(pos + ImVec2(sidebar_w, 0.0f), pos + ImVec2(sidebar_w, size.y), col(color::kBorder));

    ImGui::SetCursorPos(ImVec2(px(20.0f), px(20.0f)));
    ImGui::PushFont(style::semibold(), 17.0f);
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    ImGui::SetCursorPosX(px(20.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, color::kTextMute);
    ImGui::TextUnformatted("v" IL2CPP_BASE_VERSION);
    ImGui::PopStyleColor();

    ImGui::SetCursorPos(ImVec2(px(10.0f), px(86.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, px(2.0f)));
    for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
        ImGui::SetCursorPosX(px(10.0f));
        if (sidebar_item(tabs[i].name, i == g_tab, sidebar_w - px(20.0f))) {
            g_tab = i;
        }
    }
    ImGui::PopStyleVar();
    draw_sidebar_footer(pos, sidebar_w, size.y);

    const Tab& tab = tabs[g_tab];
    if (g_shown_tab != g_tab) {
        g_shown_tab = g_tab;
        g_tab_fade = 0.0f;
    }
    g_tab_fade = std::min(1.0f, g_tab_fade + ImGui::GetIO().DeltaTime * 6.0f);
    const float fade = ease_out(g_tab_fade);

    const float content_x = sidebar_w + px(24.0f);
    ImGui::SetCursorPos(ImVec2(content_x, px(20.0f)));
    ImGui::PushFont(style::semibold(), 21.0f);
    ImGui::TextUnformatted(tab.name);
    ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(content_x, ImGui::GetCursorPosY() + px(6.0f)));
    const ImVec2 page_size(size.x - content_x - px(14.0f), size.y - ImGui::GetCursorPosY() - px(14.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * fade);
    if (ImGui::BeginChild("##page", page_size, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground)) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (1.0f - fade) * px(8.0f));
        g_page = Page{};
        g_page.origin = ImGui::GetCursorScreenPos();
        g_page.width = ImGui::GetContentRegionAvail().x - px(10.0f);
        g_page.columns = std::clamp(tab.columns, 1, 2);
        g_page.column_y[0] = g_page.column_y[1] = g_page.origin.y;

        ImGui::PushID(g_tab);
        tab.draw();
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0.0f, px(4.0f)));
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::End();
}

bool begin_section(const char* title) {
    Page& page = g_page;
    const float gap = px(12.0f);
    page.column = page.count % page.columns;
    ++page.count;

    const float width = page.columns == 1 ? page.width : (page.width - gap) * 0.5f;
    ImGui::SetCursorScreenPos(ImVec2(page.origin.x + page.column * (width + gap), page.column_y[page.column]));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(px(14.0f), px(12.0f)));
    const bool open = ImGui::BeginChild(title, ImVec2(width, 0.0f),
        ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_NavFlattened,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    if (open) {
        ImGui::PushFont(style::semibold(), 0.0f);
        ImGui::TextUnformatted(title, ImGui::FindRenderedTextEnd(title));
        ImGui::PopFont();
        ImGui::Dummy(ImVec2(0.0f, px(2.0f)));
    }
    return open;
}

void end_section() {
    ImGui::EndChild();
    g_page.column_y[g_page.column] = ImGui::GetItemRectMax().y + px(12.0f);
}

bool toggle(const char* label, bool* value) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    const ImGuiID id = window->GetID(label);
    const float h = ImGui::GetFrameHeight();
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, pos + ImVec2(ImGui::GetContentRegionAvail().x, h));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) {
        return false;
    }

    bool hovered = false;
    bool held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) {
        *value = !*value;
        ImGui::MarkItemEdited(id);
    }

    const float on = animate(id, *value ? 1.0f : 0.0f);
    const float hov = animate(id + 1, hovered ? 1.0f : 0.0f);

    ImDrawList* dl = window->DrawList;
    label_text(bb.Min, h, label, mix(color::kTextDim, color::kText, std::max(on, hov)));

    const ImVec2 sw(px(34.0f), px(18.0f));
    const ImVec2 smin(bb.Max.x - sw.x, bb.Min.y + (h - sw.y) * 0.5f);
    const ImVec2 smax = smin + sw;
    const ImU32 off_track = mix(color::kControlPress, IM_COL32(62, 62, 70, 255), hov);
    dl->AddRectFilled(smin, smax, mix(off_track, color::kAccent, on), sw.y * 0.5f);

    const float r = sw.y * 0.5f - px(3.0f);
    const float cx = ImLerp(smin.x + sw.y * 0.5f, smax.x - sw.y * 0.5f, on);
    dl->AddCircleFilled(ImVec2(cx, smin.y + sw.y * 0.5f), r, mix(color::kTextDim, color::kOnAccent, on));
    return pressed;
}

bool slider(const char* label, float* value, float min, float max, const char* format) {
    return slider_core(label, value, min, max, false, format);
}

bool slider(const char* label, int* value, int min, int max, const char* format) {
    float v = static_cast<float>(*value);
    const bool changed = slider_core(label, &v, static_cast<float>(min), static_cast<float>(max), true, format);
    *value = static_cast<int>(v);
    return changed;
}

bool button(const char* label, float width) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    const float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, pos + ImVec2(w, ImGui::GetFrameHeight() + px(2.0f)));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) {
        return false;
    }

    bool hovered = false;
    bool held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const float hov = animate(id, hovered ? 1.0f : 0.0f);

    const float rounding = ImGui::GetStyle().FrameRounding;
    ImDrawList* dl = window->DrawList;
    dl->AddRectFilled(bb.Min, bb.Max,
        held ? col(color::kControlPress) : mix(color::kControl, color::kControlHover, hov), rounding);
    dl->AddRect(bb.Min, bb.Max, mix(color::kBorder, color::kControlPress, hov), rounding);
    ImGui::RenderTextClipped(bb.Min, bb.Max, label, nullptr, &label_size, ImVec2(0.5f, 0.5f), &bb);
    return pressed;
}

bool combo(const char* label, int* current, std::initializer_list<const char*> items) {
    return combo(label, current, items.begin(), static_cast<int>(items.size()));
}

bool combo(const char* label, int* current, const char* const* items, int count) {
    ImGui::PushID(label);
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label, ImGui::FindRenderedTextEnd(label));
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(start.x + w * 0.45f, start.y));
    ImGui::SetNextItemWidth(w * 0.55f);

    bool changed = false;
    const char* preview = (*current >= 0 && *current < count) ? items[*current] : "";
    if (ImGui::BeginCombo("##combo", preview)) {
        for (int i = 0; i < count; ++i) {
            const bool selected = i == *current;
            if (ImGui::Selectable(items[i], selected)) {
                *current = i;
                changed = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    return changed;
}

bool input(const char* hint, std::string* value) {
    ImGui::PushID(hint);
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool changed = ImGui::InputTextWithHint("##input", hint, value);
    ImGui::PopID();
    return changed;
}

bool input(const char* hint, char* buffer, std::size_t size) {
    ImGui::PushID(hint);
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool changed = ImGui::InputTextWithHint("##input", hint, buffer, size);
    ImGui::PopID();
    return changed;
}

bool color(const char* label, float rgba[4]) {
    ImGui::PushID(label);
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label, ImGui::FindRenderedTextEnd(label));
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(start.x + w - ImGui::GetFrameHeight() * 1.6f, start.y));

    const bool changed = ImGui::ColorEdit4("##color", rgba,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
        ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
    ImGui::PopID();
    return changed;
}

bool segmented(const char* id_str, int* current, std::initializer_list<const char*> items) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const int count = static_cast<int>(items.size());
    if (window->SkipItems || count == 0) {
        return false;
    }

    const ImGuiID id = window->GetID(id_str);
    const float w = ImGui::GetContentRegionAvail().x;
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, pos + ImVec2(w, ImGui::GetFrameHeight()));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) {
        return false;
    }

    const float seg_w = w / static_cast<float>(count);
    bool changed = false;
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const int hit = std::clamp(static_cast<int>((ImGui::GetIO().MousePos.x - bb.Min.x) / seg_w), 0, count - 1);
        if (hit != *current) {
            *current = hit;
            changed = true;
            ImGui::MarkItemEdited(id);
        }
    }

    const float rounding = ImGui::GetStyle().FrameRounding;
    const float inset = px(3.0f);
    ImDrawList* dl = window->DrawList;
    dl->AddRectFilled(bb.Min, bb.Max, col(color::kControl), rounding);

    const float at = animate(id, static_cast<float>(*current), 20.0f);
    const float x = bb.Min.x + at * seg_w;
    dl->AddRectFilled(ImVec2(x + inset, bb.Min.y + inset), ImVec2(x + seg_w - inset, bb.Max.y - inset),
        col(color::kControlPress), rounding - 1.0f);

    int i = 0;
    for (const char* item : items) {
        const ImRect seg(ImVec2(bb.Min.x + seg_w * i, bb.Min.y), ImVec2(bb.Min.x + seg_w * (i + 1), bb.Max.y));
        ImGui::PushStyleColor(ImGuiCol_Text, i == *current ? color::kText : color::kTextDim);
        ImGui::RenderTextClipped(seg.Min, seg.Max, item, nullptr, nullptr, ImVec2(0.5f, 0.5f), &seg);
        ImGui::PopStyleColor();
        ++i;
    }
    return changed;
}

void text(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, color::kTextDim);
    ImGui::TextWrappedV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

void value(const char* label, const char* fmt, ...) {
    char buf[256]{};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleColor(ImGuiCol_Text, color::kTextDim);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(start.x + w - ImGui::CalcTextSize(buf).x, start.y));
    ImGui::TextUnformatted(buf);
}

void separator() {
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = px(9.0f);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x, pos.y + h * 0.5f), ImVec2(pos.x + w, pos.y + h * 0.5f),
        col(color::kBorder));
    ImGui::Dummy(ImVec2(w, h));
}

void notify(const char* fmt, ...) {
    char buf[256]{};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    std::lock_guard lock(g_toast_mutex);
    g_toasts.push_back(Toast{ buf });
    while (g_toasts.size() > 5) {
        g_toasts.pop_front();
    }
}

void draw_notifications() {
    constexpr double kLife = 3.0;
    std::lock_guard lock(g_toast_mutex);
    if (g_toasts.empty()) {
        return;
    }

    const double now = ImGui::GetTime();
    while (!g_toasts.empty() && g_toasts.front().start >= 0.0 && now - g_toasts.front().start > kLife) {
        g_toasts.pop_front();
    }

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 pad(px(14.0f), px(10.0f));
    const float rounding = px(8.0f);
    float y = display.y - px(24.0f);

    for (auto it = g_toasts.rbegin(); it != g_toasts.rend(); ++it) {
        if (it->start < 0.0) {
            it->start = now;
        }
        const float age = static_cast<float>(now - it->start);
        const float in = ease_out(ImSaturate(age / 0.18f));
        const float alpha = in * ImSaturate((static_cast<float>(kLife) - age) / 0.35f);

        const ImVec2 text_size = ImGui::CalcTextSize(it->text.c_str());
        const ImVec2 box(text_size.x + pad.x * 2.0f + px(14.0f), text_size.y + pad.y * 2.0f);
        const ImVec2 min(display.x - px(24.0f) - box.x + (1.0f - in) * px(18.0f), y - box.y);
        const ImVec2 max = min + box;

        dl->AddRectFilled(min, max, with_alpha(color::kPanel, alpha), rounding);
        dl->AddRect(min, max, with_alpha(color::kBorder, alpha), rounding);
        dl->AddCircleFilled(ImVec2(min.x + pad.x + px(3.0f), min.y + box.y * 0.5f), px(3.0f),
            with_alpha(color::kAccent, alpha));
        dl->AddText(ImVec2(min.x + pad.x + px(14.0f), min.y + pad.y), with_alpha(color::kText, alpha),
            it->text.c_str());
        y = min.y - px(8.0f);
    }
}

} // namespace il2cpp_base::ui
