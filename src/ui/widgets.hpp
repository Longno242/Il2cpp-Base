#pragma once

#include <imgui.h>

#include <cstddef>
#include <initializer_list>
#include <span>
#include <string>

namespace il2cpp_base::ui {

struct Tab {
    const char* name;
    void (*draw)();
    int columns = 2;
};

// The menu window: tabs in a sidebar on the left, the selected tab on the right.
void window(const char* title, std::span<const Tab> tabs);

// Cards that a tab is made of. They fill the tab's columns left to right, then wrap.
bool begin_section(const char* title);
void end_section();

template <typename Body>
void section(const char* title, Body&& body) {
    if (begin_section(title)) {
        body();
    }
    end_section();
}

bool toggle(const char* label, bool* value);
bool slider(const char* label, float* value, float min, float max, const char* format = "%.2f");
bool slider(const char* label, int* value, int min, int max, const char* format = "%d");
// width <= 0 fills the row.
bool button(const char* label, float width = 0.0f);
bool combo(const char* label, int* current, std::initializer_list<const char*> items);
bool combo(const char* label, int* current, const char* const* items, int count);
bool input(const char* hint, std::string* value);
bool input(const char* hint, char* buffer, std::size_t size);
bool color(const char* label, float rgba[4]);
bool segmented(const char* id, int* current, std::initializer_list<const char*> items);

void text(const char* fmt, ...) IM_FMTARGS(1);
void value(const char* label, const char* fmt, ...) IM_FMTARGS(2);
void separator();

// Thread-safe. Shows a short message in the bottom-right corner, even with the menu closed.
void notify(const char* fmt, ...) IM_FMTARGS(1);
void draw_notifications();

} // namespace il2cpp_base::ui
