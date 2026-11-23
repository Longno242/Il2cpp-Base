#pragma once

// Keeps Unity's cursor free while the menu is open. Main thread only.
namespace il2cpp_base::ui::cursor {

void install();
void update();

} // namespace il2cpp_base::ui::cursor
