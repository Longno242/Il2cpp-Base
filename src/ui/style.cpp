#include "ui/style.hpp"

#include <Windows.h>

#include <algorithm>
#include <string>

namespace il2cpp_base::ui::style {
namespace {

ImFont* g_regular = nullptr;
ImFont* g_semibold = nullptr;
float g_scale = 1.0f;
float g_pending_scale = 1.0f;
bool g_dirty = true;

ImVec4 vec(ImU32 c) {
    return ImGui::ColorConvertU32ToFloat4(c);
}

ImVec4 vec(ImU32 c, float alpha) {
    ImVec4 v = vec(c);
    v.w = alpha;
    return v;
}

std::string fonts_dir() {
    wchar_t windir[MAX_PATH]{};
    if (!GetWindowsDirectoryW(windir, MAX_PATH)) {
        return {};
    }
    char utf8[MAX_PATH * 3]{};
    WideCharToMultiByte(CP_UTF8, 0, windir, -1, utf8, sizeof(utf8), nullptr, nullptr);
    return std::string(utf8) + "\\Fonts\\";
}

ImFont* add_system_font(const std::string& dir, const char* file, float size) {
    const std::string path = dir + file;
    if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return nullptr;
    }
    ImFontConfig cfg;
    cfg.Flags |= ImFontFlags_NoLoadError;
    cfg.RasterizerMultiply = 1.1f;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), size, &cfg);
}

ImGuiStyle base_style() {
    ImGuiStyle s;

    s.WindowPadding = ImVec2(14.0f, 14.0f);
    s.WindowRounding = 10.0f;
    s.WindowBorderSize = 1.0f;
    s.ChildRounding = 8.0f;
    s.ChildBorderSize = 1.0f;
    s.PopupRounding = 8.0f;
    s.PopupBorderSize = 1.0f;
    s.FramePadding = ImVec2(10.0f, 6.0f);
    s.FrameRounding = 6.0f;
    s.FrameBorderSize = 0.0f;
    s.ItemSpacing = ImVec2(8.0f, 8.0f);
    s.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    s.CellPadding = ImVec2(8.0f, 5.0f);
    s.IndentSpacing = 16.0f;
    s.ScrollbarSize = 8.0f;
    s.ScrollbarRounding = 8.0f;
    s.ScrollbarPadding = 0.0f;
    s.GrabMinSize = 10.0f;
    s.GrabRounding = 6.0f;
    s.TabRounding = 6.0f;
    s.TabBorderSize = 0.0f;
    s.TabBarBorderSize = 0.0f;
    s.TabBarOverlineSize = 0.0f;
    s.SeparatorSize = 1.0f;
    s.SelectableTextAlign = ImVec2(0.0f, 0.5f);
    s.WindowTitleAlign = ImVec2(0.0f, 0.5f);

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = vec(color::kText);
    c[ImGuiCol_TextDisabled] = vec(color::kTextMute);
    c[ImGuiCol_WindowBg] = vec(color::kBackground);
    c[ImGuiCol_ChildBg] = vec(color::kPanel);
    c[ImGuiCol_PopupBg] = vec(color::kPanel);
    c[ImGuiCol_Border] = vec(color::kBorder);
    c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg] = vec(color::kControl);
    c[ImGuiCol_FrameBgHovered] = vec(color::kControlHover);
    c[ImGuiCol_FrameBgActive] = vec(color::kControlPress);
    c[ImGuiCol_TitleBg] = vec(color::kSidebar);
    c[ImGuiCol_TitleBgActive] = vec(color::kSidebar);
    c[ImGuiCol_TitleBgCollapsed] = vec(color::kSidebar);
    c[ImGuiCol_MenuBarBg] = vec(color::kSidebar);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = vec(color::kControlPress);
    c[ImGuiCol_ScrollbarGrabHovered] = vec(color::kTextMute);
    c[ImGuiCol_ScrollbarGrabActive] = vec(color::kTextDim);
    c[ImGuiCol_CheckMark] = vec(color::kAccent);
    c[ImGuiCol_CheckboxSelectedBg] = vec(color::kControlPress);
    c[ImGuiCol_SliderGrab] = vec(color::kTextDim);
    c[ImGuiCol_SliderGrabActive] = vec(color::kAccent);
    c[ImGuiCol_Button] = vec(color::kControl);
    c[ImGuiCol_ButtonHovered] = vec(color::kControlHover);
    c[ImGuiCol_ButtonActive] = vec(color::kControlPress);
    c[ImGuiCol_Header] = vec(color::kControl);
    c[ImGuiCol_HeaderHovered] = vec(color::kControlHover);
    c[ImGuiCol_HeaderActive] = vec(color::kControlPress);
    c[ImGuiCol_Separator] = vec(color::kBorder);
    c[ImGuiCol_SeparatorHovered] = vec(color::kTextMute);
    c[ImGuiCol_SeparatorActive] = vec(color::kTextDim);
    c[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ResizeGripHovered] = vec(color::kControlPress);
    c[ImGuiCol_ResizeGripActive] = vec(color::kTextMute);
    c[ImGuiCol_InputTextCursor] = vec(color::kText);
    c[ImGuiCol_Tab] = vec(color::kControl);
    c[ImGuiCol_TabHovered] = vec(color::kControlHover);
    c[ImGuiCol_TabSelected] = vec(color::kControlPress);
    c[ImGuiCol_TabSelectedOverline] = vec(color::kAccent);
    c[ImGuiCol_TabDimmed] = vec(color::kControl);
    c[ImGuiCol_TabDimmedSelected] = vec(color::kControlHover);
    c[ImGuiCol_TabDimmedSelectedOverline] = vec(color::kTextDim);
    c[ImGuiCol_PlotLines] = vec(color::kTextDim);
    c[ImGuiCol_PlotLinesHovered] = vec(color::kAccent);
    c[ImGuiCol_PlotHistogram] = vec(color::kTextDim);
    c[ImGuiCol_PlotHistogramHovered] = vec(color::kAccent);
    c[ImGuiCol_TableHeaderBg] = vec(color::kSidebar);
    c[ImGuiCol_TableBorderStrong] = vec(color::kBorder);
    c[ImGuiCol_TableBorderLight] = vec(color::kControl);
    c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt] = vec(color::kAccent, 0.02f);
    c[ImGuiCol_TextLink] = vec(color::kText);
    c[ImGuiCol_TextSelectedBg] = vec(color::kAccent, 0.18f);
    c[ImGuiCol_DragDropTarget] = vec(color::kAccent);
    c[ImGuiCol_NavCursor] = vec(color::kTextDim);
    c[ImGuiCol_NavWindowingHighlight] = vec(color::kAccent, 0.7f);
    c[ImGuiCol_NavWindowingDimBg] = ImVec4(0, 0, 0, 0.5f);
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.6f);

    return s;
}

} // namespace

void load_fonts() {
    const std::string dir = fonts_dir();
    g_regular = add_system_font(dir, "segoeui.ttf", 15.0f);
    g_semibold = add_system_font(dir, "seguisb.ttf", 15.0f);

    ImGuiIO& io = ImGui::GetIO();
    if (!g_regular) {
        g_regular = io.Fonts->AddFontDefaultVector();
    }
    if (!g_semibold) {
        g_semibold = g_regular;
    }
    io.FontDefault = g_regular;
}

ImFont* regular() {
    return g_regular;
}

ImFont* semibold() {
    return g_semibold;
}

void set_scale(float scale) {
    g_pending_scale = std::clamp(scale, 0.75f, 2.0f);
    g_dirty = true;
}

float scale() {
    return g_scale;
}

void apply_pending() {
    if (!g_dirty) {
        return;
    }
    g_dirty = false;
    g_scale = g_pending_scale;

    ImGuiStyle& style = ImGui::GetStyle();
    style = base_style();
    style.ScaleAllSizes(g_scale);
    style.FontScaleMain = g_scale;
}

} // namespace il2cpp_base::ui::style
