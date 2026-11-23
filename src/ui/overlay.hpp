#pragma once

namespace il2cpp_base::ui {

struct Options {
    bool watermark = true;
    bool software_cursor = true;
    bool block_input = true;
    bool unlock_cursor = true;
};

Options& options();

} // namespace il2cpp_base::ui

namespace il2cpp_base::ui::overlay {

bool install();
void uninstall();

bool menu_open();
void set_menu_open(bool open);
const char* menu_key_name();

} // namespace il2cpp_base::ui::overlay
