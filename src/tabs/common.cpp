#include "tabs/tabs.hpp"

#include <imgui.h>

#include "features/field_override.hpp"
#include "ui/style.hpp"
#include "ui/widgets.hpp"

namespace il2cpp_base::tabs {

void field_override(const char* label, features::FieldOverride& field, float min, float max, const char* format) {
    ImGui::PushID(label);
    ui::toggle(label, &field.enabled);
    if (field.enabled) {
        ui::slider("Value", &field.value, min, max, format);
        ui::input("Field name", field.field, sizeof(field.field));
        ImGui::PushStyleColor(ImGuiCol_Text, ui::style::color::kTextMute);
        ImGui::Text("%s", field.status);
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }
    ImGui::PopID();
}

} // namespace il2cpp_base::tabs
