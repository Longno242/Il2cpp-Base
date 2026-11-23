#pragma once

#include <imgui.h>

namespace il2cpp_base::ui::style {

namespace color {

inline constexpr ImU32 kBackground   = IM_COL32(11, 11, 12, 255);
inline constexpr ImU32 kSidebar      = IM_COL32(15, 15, 17, 255);
inline constexpr ImU32 kPanel        = IM_COL32(19, 19, 21, 255);
inline constexpr ImU32 kBorder       = IM_COL32(32, 32, 36, 255);
inline constexpr ImU32 kControl      = IM_COL32(28, 28, 32, 255);
inline constexpr ImU32 kControlHover = IM_COL32(38, 38, 43, 255);
inline constexpr ImU32 kControlPress = IM_COL32(48, 48, 54, 255);
inline constexpr ImU32 kText         = IM_COL32(237, 237, 239, 255);
inline constexpr ImU32 kTextDim      = IM_COL32(150, 150, 158, 255);
inline constexpr ImU32 kTextMute     = IM_COL32(92, 92, 100, 255);
inline constexpr ImU32 kAccent       = IM_COL32(244, 244, 245, 255);
inline constexpr ImU32 kOnAccent     = IM_COL32(11, 11, 12, 255);

} // namespace color

void load_fonts();
ImFont* regular();
ImFont* semibold();

// Scale changes are queued and applied by apply_pending() before the next ImGui frame.
void set_scale(float scale);
float scale();
void apply_pending();

inline float px(float value) {
    return value * scale();
}

} // namespace il2cpp_base::ui::style
