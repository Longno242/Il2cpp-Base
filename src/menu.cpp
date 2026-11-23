#include "menu.hpp"

#include "config.hpp"
#include "tabs/tabs.hpp"
#include "ui/pages.hpp"
#include "ui/widgets.hpp"

namespace il2cpp_base::menu {
namespace {

// Sidebar order. The third field is the column count (2 by default, 1 for full-width pages).
constexpr ui::Tab kTabs[] = {
    { "Home", tabs::home },
    { "Player", tabs::player },
    { "Movement", tabs::movement },
    { "Player List", tabs::player_list, 1 },
    { "Visuals", tabs::visuals },
    { "World", tabs::world },
    { "Explorer", ui::explorer_page, 1 },
    { "Log", ui::log_page, 1 },
    { "Settings", tabs::settings },
};

} // namespace

void draw() {
    ui::window(config::kName, kTabs);
}

} // namespace il2cpp_base::menu
