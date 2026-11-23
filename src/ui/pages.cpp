#include "ui/pages.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/logger.hpp"
#include "core/main_thread.hpp"
#include "il2cpp/resolver.hpp"
#include "ui/style.hpp"
#include "ui/widgets.hpp"

namespace il2cpp_base::ui {
namespace {

bool contains_nocase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    }) != haystack.end();
}

void copy(const std::string& text) {
    ImGui::SetClipboardText(text.c_str());
    notify("Copied %s", text.c_str());
}

// ---------------------------------------------------------------------------------------------
// Explorer. IL2CPP metadata is only touched on the main thread; the UI reads immutable snapshots.

struct ClassItem {
    std::string label;
    il2cpp::Class* klass = nullptr;
};

struct Member {
    std::string type;
    std::string name;
    std::string where;
};

struct ClassView {
    std::string title;
    std::vector<Member> fields;
    std::vector<Member> methods;
};

struct Explorer {
    std::mutex mutex;
    bool requested = false;
    std::shared_ptr<const std::vector<std::string>> assemblies;
    std::shared_ptr<const std::vector<ClassItem>> classes;
    std::shared_ptr<const ClassView> view;
    bool loading = false;
    unsigned generation = 0;

    int assembly = -1;
    int selected = -1;
    int member_tab = 0;
    std::string class_filter;
    std::string member_filter;
};

Explorer ex;

void request_assemblies() {
    ex.requested = true;
    main_thread::post([] {
        auto names = std::make_shared<std::vector<std::string>>();
        for (il2cpp::Image* image : il2cpp::images()) {
            names->push_back(image->name());
        }
        std::sort(names->begin(), names->end());
        std::lock_guard lock(ex.mutex);
        ex.assemblies = std::move(names);
    });
}

void request_classes(const std::string& assembly) {
    const unsigned generation = ++ex.generation;
    ex.loading = true;
    ex.classes.reset();
    ex.view.reset();
    ex.selected = -1;

    main_thread::post([assembly, generation] {
        auto items = std::make_shared<std::vector<ClassItem>>();
        if (il2cpp::Image* image = il2cpp::image(assembly)) {
            for (il2cpp::Class* klass : image->classes()) {
                std::string label = klass->namespaze().empty() ? klass->name()
                                                               : klass->namespaze() + "." + klass->name();
                items->push_back({ std::move(label), klass });
            }
        }
        std::sort(items->begin(), items->end(), [](const ClassItem& a, const ClassItem& b) {
            return a.label < b.label;
        });

        std::lock_guard lock(ex.mutex);
        if (generation == ex.generation) {
            ex.classes = std::move(items);
            ex.loading = false;
        }
    });
}

void request_view(const ClassItem& item) {
    const unsigned generation = ++ex.generation;
    ex.loading = true;
    ex.view.reset();

    main_thread::post([item, generation] {
        auto view = std::make_shared<ClassView>();
        view->title = item.label;
        char buffer[64]{};

        for (il2cpp::Field* field : item.klass->fields()) {
            snprintf(buffer, sizeof(buffer), field->is_static() ? "static" : "0x%zX", field->offset());
            view->fields.push_back({ field->type(), field->name(), buffer });
        }
        for (il2cpp::Method* method : item.klass->methods()) {
            std::string signature = method->name() + "(";
            for (std::size_t i = 0; i < method->params().size(); ++i) {
                const auto& [type, name] = method->params()[i];
                signature += (i ? ", " : "") + type + " " + name;
            }
            signature += ")";
            snprintf(buffer, sizeof(buffer), "0x%llX", static_cast<unsigned long long>(method->rva()));
            std::string type = method->is_static() ? "static " + method->return_type() : method->return_type();
            view->methods.push_back({ std::move(type), std::move(signature), buffer });
        }

        std::lock_guard lock(ex.mutex);
        if (generation == ex.generation) {
            ex.view = std::move(view);
            ex.loading = false;
        }
    });
}

void draw_member_table(const std::vector<Member>& members, const char* where_header) {
    const ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
    if (!ImGui::BeginTable("##members", 3, flags)) {
        return;
    }
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_None, 0.35f);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0.45f);
    ImGui::TableSetupColumn(where_header, ImGuiTableColumnFlags_None, 0.20f);
    ImGui::TableHeadersRow();

    std::vector<const Member*> rows;
    rows.reserve(members.size());
    for (const Member& m : members) {
        if (contains_nocase(m.name, ex.member_filter) || contains_nocase(m.type, ex.member_filter)) {
            rows.push_back(&m);
        }
    }

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(rows.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
            const Member& m = *rows[i];
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, style::color::kTextDim);
            ImGui::TextUnformatted(m.type.c_str());
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            if (ImGui::Selectable(m.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                copy(m.name);
            }
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, style::color::kTextMute);
            ImGui::TextUnformatted(m.where.c_str());
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }
    ImGui::EndTable();
}

// ---------------------------------------------------------------------------------------------
// Log

struct LogView {
    unsigned long long version = ~0ull;
    std::vector<log::Line> lines;
    std::string filter;
    bool follow = true;
};

LogView lv;

ImU32 level_color(log::Level level) {
    switch (level) {
    case log::Level::Warn: return IM_COL32(226, 192, 120, 255);
    case log::Level::Error:
    case log::Level::Fatal: return IM_COL32(232, 120, 120, 255);
    case log::Level::Debug: return style::color::kTextMute;
    default: return style::color::kTextDim;
    }
}

} // namespace

void explorer_page() {
    if (!il2cpp::ready()) {
        text("IL2CPP is not ready yet.");
        return;
    }

    std::shared_ptr<const std::vector<std::string>> assemblies;
    std::shared_ptr<const std::vector<ClassItem>> classes;
    std::shared_ptr<const ClassView> view;
    bool loading = false;
    {
        std::lock_guard lock(ex.mutex);
        if (!ex.requested) {
            request_assemblies();
        }
        assemblies = ex.assemblies;
        classes = ex.classes;
        view = ex.view;
        loading = ex.loading;
    }

    const float gap = style::px(12.0f);
    const float list_w = style::px(250.0f);
    const float height = ImGui::GetContentRegionAvail().y - style::px(4.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style::px(10.0f), style::px(10.0f)));

    ImGui::BeginChild("##classes", ImVec2(list_w, height), ImGuiChildFlags_Borders);
    {
        std::vector<const char*> names;
        if (assemblies) {
            for (const std::string& name : *assemblies) {
                names.push_back(name.c_str());
            }
        }
        ImGui::SetNextItemWidth(-FLT_MIN);
        const char* preview = ex.assembly >= 0 && ex.assembly < static_cast<int>(names.size())
            ? names[ex.assembly] : "Pick an assembly";
        if (ImGui::BeginCombo("##assembly", preview, ImGuiComboFlags_HeightLarge)) {
            for (int i = 0; i < static_cast<int>(names.size()); ++i) {
                if (ImGui::Selectable(names[i], i == ex.assembly)) {
                    ex.assembly = i;
                    std::lock_guard lock(ex.mutex);
                    request_classes(names[i]);
                }
            }
            ImGui::EndCombo();
        }
        input("Search classes", &ex.class_filter);
        ImGui::Spacing();

        if (classes) {
            ImGui::BeginChild("##class_list", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
            std::vector<int> visible;
            for (int i = 0; i < static_cast<int>(classes->size()); ++i) {
                if (contains_nocase((*classes)[i].label, ex.class_filter)) {
                    visible.push_back(i);
                }
            }
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(visible.size()));
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                    const int i = visible[row];
                    ImGui::PushID(i);
                    if (ImGui::Selectable((*classes)[i].label.c_str(), i == ex.selected)) {
                        ex.selected = i;
                        std::lock_guard lock(ex.mutex);
                        request_view((*classes)[i]);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
        } else if (loading) {
            text("Loading...");
        }
    }
    ImGui::EndChild();

    ImGui::SameLine(0.0f, gap);
    ImGui::BeginChild("##members_panel", ImVec2(0.0f, height), ImGuiChildFlags_Borders);
    if (view) {
        ImGui::PushFont(style::semibold(), 0.0f);
        ImGui::TextUnformatted(view->title.c_str());
        ImGui::PopFont();
        ImGui::Spacing();

        const std::string fields_label = "Fields  " + std::to_string(view->fields.size());
        const std::string methods_label = "Methods  " + std::to_string(view->methods.size());
        segmented("##member_tab", &ex.member_tab, { fields_label.c_str(), methods_label.c_str() });
        input("Filter", &ex.member_filter);
        ImGui::Spacing();

        if (ex.member_tab == 0) {
            draw_member_table(view->fields, "Offset");
        } else {
            draw_member_table(view->methods, "RVA");
        }
    } else {
        text(loading ? "Loading..." : "Pick a class to see its fields and methods. Click a row to copy it.");
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
}

void log_page() {
    log::Logger& logger = log::Logger::instance();
    if (const auto version = logger.version(); version != lv.version) {
        lv.version = version;
        lv.lines = logger.history();
    }

    const float button_w = style::px(80.0f);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - button_w * 2.0f - style::px(16.0f));
    ImGui::InputTextWithHint("##log_filter", "Filter", &lv.filter);
    ImGui::SameLine(0.0f, style::px(8.0f));
    if (button(lv.follow ? "Following" : "Follow", button_w)) {
        lv.follow = !lv.follow;
    }
    ImGui::SameLine(0.0f, style::px(8.0f));
    if (button("Clear", button_w)) {
        logger.clear_history();
    }
    ImGui::Spacing();

    std::vector<int> visible;
    visible.reserve(lv.lines.size());
    for (int i = 0; i < static_cast<int>(lv.lines.size()); ++i) {
        if (contains_nocase(lv.lines[i].text, lv.filter)) {
            visible.push_back(i);
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style::px(10.0f), style::px(8.0f)));
    ImGui::BeginChild("##log", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - style::px(4.0f)),
        ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PopStyleVar();

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(visible.size()));
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const log::Line& line = lv.lines[visible[row]];
            ImGui::PushStyleColor(ImGuiCol_Text, level_color(line.level));
            ImGui::TextUnformatted(line.text.c_str());
            ImGui::PopStyleColor();
        }
    }
    if (lv.follow && ImGui::GetScrollY() < ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

} // namespace il2cpp_base::ui
