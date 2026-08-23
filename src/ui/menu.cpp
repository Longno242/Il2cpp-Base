#include "il2cpp_base/ui/menu.hpp"

#include <cstdio>

#include <imgui.h>

#include "il2cpp_base/core/config.hpp"
#include "il2cpp_base/features/example_hook.hpp"
#include "il2cpp_base/il2cpp/resolver.hpp"
#include "il2cpp_base/ui/theme.hpp"

namespace il2cpp_base::ui {
namespace {

char g_lookup_assembly[128] = "Assembly-CSharp.dll";
char g_lookup_namespace[128]{};
char g_lookup_class[128]{};
il2cpp::Class* g_lookup_result = nullptr;
char g_lookup_status[256] = "assembly + class, then resolve";
int g_sheet = 0;

void draw_overview() {
    theme::kv("runtime", il2cpp::ready() ? "attached" : "waiting");
    theme::kv("log", config::kLogFileName);
    theme::kv("toggle", "INSERT");

    ImGui::Dummy(ImVec2(1.0f, 6.0f));
    theme::hairline();
    ImGui::TextWrapped(
        "Survey plate for IL2CPP Unity games. Put feature work in src/features/. "
        "Keep runtime_invoke on the WndProc pump, not Present.");
}

void draw_catalog() {
    if (!il2cpp::ready()) {
        ImGui::TextDisabled("wait until GameAssembly.dll is mapped");
        return;
    }

    char count[32]{};
    snprintf(count, sizeof(count), "%zu", il2cpp::images().size());
    theme::kv("images", count);

    theme::begin_inset("##assemblies", 120.0f);
    for (const il2cpp::Image* img : il2cpp::images()) {
        ImGui::TextUnformatted(img->name().c_str());
    }
    theme::end_inset();

    ImGui::Dummy(ImVec2(1.0f, 4.0f));
    ImGui::TextDisabled("resolve");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##asm", "image", g_lookup_assembly, sizeof(g_lookup_assembly));
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##ns", "namespace (optional)", g_lookup_namespace, sizeof(g_lookup_namespace));
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##cls", "class", g_lookup_class, sizeof(g_lookup_class));

    if (ImGui::Button("resolve", ImVec2(-1, 0))) {
        g_lookup_result = nullptr;
        if (!g_lookup_assembly[0] || !g_lookup_class[0]) {
            snprintf(g_lookup_status, sizeof(g_lookup_status), "need image + class");
        } else {
            g_lookup_result = il2cpp::find_class(g_lookup_assembly, g_lookup_class, g_lookup_namespace);
            snprintf(g_lookup_status, sizeof(g_lookup_status),
                g_lookup_result ? "on plate" : "not in '%s'", g_lookup_assembly);
        }
    }

    ImGui::TextDisabled("%s", g_lookup_status);

    if (!g_lookup_result) {
        return;
    }

    ImGui::TextWrapped("%s.%s", g_lookup_result->namespaze().c_str(), g_lookup_result->name().c_str());

    theme::begin_inset("##class_info", 160.0f);
    ImGui::TextDisabled("fields");
    for (const il2cpp::Field* field : g_lookup_result->fields()) {
        ImGui::TextUnformatted(field->describe().c_str());
    }
    ImGui::Dummy(ImVec2(1.0f, 6.0f));
    ImGui::TextDisabled("methods");
    for (const il2cpp::Method* method : g_lookup_result->methods()) {
        ImGui::TextUnformatted(method->describe().c_str());
    }
    theme::end_inset();
}

} // namespace

void tick_features() {
    features::tick();
}

void pump_main_thread_features() {
    // Present posts WM_APP via Overlay::request_main_thread_pump().
    // Put IL2CPP invokes for your features in this path (not in Present).
}

void draw_menu() {
    theme::header(config::kMenuTitle, "survey plate", "il2cpp");

    static const char* kSheets[] = { "overview", "bindings", "catalog" };
    g_sheet = theme::index_bar(kSheets, 3, g_sheet);

    if (g_sheet == 0) {
        draw_overview();
    } else if (g_sheet == 1) {
        features::draw_hooks_tab();
    } else {
        draw_catalog();
    }

    theme::footer("field plate  ·  offline use");
}

} // namespace il2cpp_base::ui
