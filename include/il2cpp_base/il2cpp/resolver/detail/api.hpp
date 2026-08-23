#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "il2cpp_base/core/config.hpp"
#include "il2cpp_base/core/logger.hpp"
#include "il2cpp_base/il2cpp/resolver/detail/exports.hpp"
#include "il2cpp_base/il2cpp/resolver/detail/scan.hpp"
#include "il2cpp_base/il2cpp/resolver/detail/spoofer.hpp"
#include "il2cpp_base/il2cpp/resolver/types.hpp"

namespace il2cpp_base::il2cpp::detail {

struct ApiTable {
    void* (*domain_get)() = nullptr;
    void* (*thread_attach)(void* domain) = nullptr;
    const void** (*get_assemblies)(const void* domain, std::size_t* size) = nullptr;
    const void* (*get_image)(const void* assembly) = nullptr;
    const char* (*get_image_name)(const void* image) = nullptr;
    std::size_t (*get_class_count)(const void* image) = nullptr;
    const void* (*get_class)(const void* image, std::size_t index) = nullptr;
    void* (*get_class_by_name)(const void* image, const char* namespaze, const char* name) = nullptr;
    const char* (*get_class_name)(void* klass) = nullptr;
    const char* (*get_class_namespace)(void* klass) = nullptr;
    bool (*is_class_enum)(const void* klass) = nullptr;
    bool (*is_class_valuetype)(const void* klass) = nullptr;
    bool (*is_class_generic)(const void* klass) = nullptr;
    void* (*get_fields)(void* klass, void** iter) = nullptr;
    void* (*get_field_by_name)(void* klass, const char* name) = nullptr;
    const char* (*get_field_name)(void* field) = nullptr;
    int (*get_field_flags)(void* field) = nullptr;
    std::size_t (*get_field_offset)(void* field) = nullptr;
    const void* (*get_field_type)(void* field) = nullptr;
    char* (*get_type_name)(const void* type) = nullptr;
    const void* (*get_method_by_name)(void* klass, const char* name, int args_count) = nullptr;
    const void* (*get_methods)(void* klass, void** iter) = nullptr;
    const char* (*get_method_name)(const void* method) = nullptr;
    const void* (*get_method_return_type)(const void* method) = nullptr;
    std::uint32_t (*get_method_param_count)(const void* method) = nullptr;
    const char* (*get_method_param_name)(const void* method, std::uint32_t index) = nullptr;
    const void* (*get_method_param_type)(const void* method, std::uint32_t index) = nullptr;
    std::uint32_t (*get_method_flags)(const void* method, std::uint32_t* iflags) = nullptr;
    bool (*is_method_instance)(const void* method) = nullptr;
    void* (*get_class_nested_types)(void* klass, void** iter) = nullptr;
    void (*free_memory)(void* ptr) = nullptr;
    void (*field_static_get_value)(void* field, void* dest) = nullptr;
    void (*field_static_set_value)(void* field, void* value) = nullptr;
    void* (*runtime_invoke)(const void* method, void* obj, void** params, void** exc) = nullptr;
    void* (*object_new)(void* klass) = nullptr;
    void* (*string_new)(const char* str) = nullptr;
    const wchar_t* (*string_chars)(void* str) = nullptr;
    int (*string_length)(void* str) = nullptr;
    void* (*object_unbox)(void* obj) = nullptr;
    void* (*value_box)(void* klass, void* data) = nullptr;
    void* (*object_get_class)(void* obj) = nullptr;
    void* (*object_to_string)(void* obj, void** exc) = nullptr;
    void (*runtime_class_init)(void* klass) = nullptr;
    void* (*thread_current)() = nullptr;
};

struct Context {
    void* jmp_rdx = nullptr;
    bool initialized = false;
    std::uint32_t method_pointer_offset = 0;
    ApiTable api{};
    std::vector<std::unique_ptr<Image>> images;
    std::unordered_map<std::string, Image*> image_lookup;
};

inline Context& ctx() {
    static Context instance;
    return instance;
}

inline bool resolve_api() {
    auto& c = ctx();
    build_export_cache();

#define RESOLVE_PATTERN(field, pattern, mask) \
    c.api.field = reinterpret_cast<decltype(c.api.field)>( \
        scan_module(config::kGameAssemblyModule, pattern, mask)); \
    if (!c.api.field) LOG_ERROR("IL2CPP", "Missing pattern: " #field)

#define RESOLVE_EXPORT(field, export_name) \
    c.api.field = reinterpret_cast<decltype(c.api.field)>(find_export_by_name(export_name)); \
    if (!c.api.field) LOG_ERROR("IL2CPP", "Missing export: " export_name)

#define RESOLVE_EXPORT_OPTIONAL(field, export_name) \
    c.api.field = reinterpret_cast<decltype(c.api.field)>(find_export_by_name(export_name)); \
    if (!c.api.field) LOG_DEBUG("IL2CPP", "Optional export missing: " export_name)

    c.api.domain_get = reinterpret_cast<decltype(c.api.domain_get)>(
        find_export_by_name("il2cpp_domain_get"));
    if (!c.api.domain_get) {
        RESOLVE_PATTERN(domain_get,
            "\x48\x83\xEC\x00\x48\x63\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00"
            "\x4C\x8B\x05\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x8B\x0C\x08\x48\x8B\x44\x24\x00"
            "\x48\xC1\xE1\x00\x48\x03\xCA\x48\x3B\xC1\x73\x00\x48\x8B\x44\x24\x00\x48\x3B\xC2\x73\x00"
            "\x49\x63\x40\x00\x42\x8B\x4C\x00\x00\x48\x8B\x44\x24\x00\x49\x8D\x14\x88\x48\x3B\xC2\x0F\x83",
            "xxx?xxx????xxx????xxx????xxx????xxxxxxx?xxx?xxxxxxx?xxxx?xxxx?xxx?xxxx?xxxx?xxxxxxxxx");
    }

    c.api.thread_attach = reinterpret_cast<decltype(c.api.thread_attach)>(
        find_export_by_name("il2cpp_thread_attach"));
    if (!c.api.thread_attach) {
        RESOLVE_PATTERN(thread_attach,
            "\x40\x56\x48\x83\xEC\x00\x48\x8B\xF1\x48\x8B\x0D\x00\x00\x00\x00\x8B\x09",
            "xxxxx?xxxxxx????xx");
    }

    RESOLVE_EXPORT(get_assemblies, "il2cpp_domain_get_assemblies");
    RESOLVE_EXPORT(get_image, "il2cpp_assembly_get_image");
    RESOLVE_EXPORT(get_image_name, "il2cpp_image_get_name");
    RESOLVE_EXPORT(get_class_count, "il2cpp_image_get_class_count");
    RESOLVE_EXPORT(get_class_by_name, "il2cpp_class_from_name");
    RESOLVE_EXPORT(get_class, "il2cpp_image_get_class");
    RESOLVE_EXPORT(get_class_name, "il2cpp_class_get_name");
    RESOLVE_EXPORT(get_class_namespace, "il2cpp_class_get_namespace");
    RESOLVE_EXPORT(is_class_enum, "il2cpp_class_is_enum");
    RESOLVE_EXPORT(is_class_valuetype, "il2cpp_class_is_valuetype");
    RESOLVE_EXPORT(is_class_generic, "il2cpp_class_is_generic");
    RESOLVE_EXPORT(get_fields, "il2cpp_class_get_fields");
    RESOLVE_EXPORT(get_field_by_name, "il2cpp_class_get_field_from_name");
    RESOLVE_EXPORT(get_field_name, "il2cpp_field_get_name");
    RESOLVE_EXPORT(get_field_flags, "il2cpp_field_get_flags");
    RESOLVE_EXPORT(get_field_offset, "il2cpp_field_get_offset");
    RESOLVE_EXPORT(get_field_type, "il2cpp_field_get_type");
    RESOLVE_EXPORT(get_type_name, "il2cpp_type_get_name");
    RESOLVE_EXPORT(get_method_by_name, "il2cpp_class_get_method_from_name");
    RESOLVE_EXPORT(get_methods, "il2cpp_class_get_methods");
    RESOLVE_EXPORT(get_method_name, "il2cpp_method_get_name");
    RESOLVE_EXPORT(get_method_return_type, "il2cpp_method_get_return_type");
    RESOLVE_EXPORT(get_method_param_count, "il2cpp_method_get_param_count");
    RESOLVE_EXPORT(get_method_param_name, "il2cpp_method_get_param_name");
    RESOLVE_EXPORT(get_method_param_type, "il2cpp_method_get_param");
    RESOLVE_EXPORT(free_memory, "il2cpp_free");
    RESOLVE_EXPORT(get_method_flags, "il2cpp_method_get_flags");
    RESOLVE_EXPORT(is_method_instance, "il2cpp_method_is_instance");
    RESOLVE_EXPORT(get_class_nested_types, "il2cpp_class_get_nested_types");
    RESOLVE_EXPORT_OPTIONAL(field_static_get_value, "il2cpp_field_static_get_value");
    RESOLVE_EXPORT_OPTIONAL(field_static_set_value, "il2cpp_field_static_set_value");
    RESOLVE_EXPORT_OPTIONAL(runtime_invoke, "il2cpp_runtime_invoke");
    RESOLVE_EXPORT_OPTIONAL(object_new, "il2cpp_object_new");
    RESOLVE_EXPORT_OPTIONAL(string_new, "il2cpp_string_new");
    RESOLVE_EXPORT_OPTIONAL(string_chars, "il2cpp_string_chars");
    RESOLVE_EXPORT_OPTIONAL(string_length, "il2cpp_string_length");
    RESOLVE_EXPORT_OPTIONAL(object_unbox, "il2cpp_object_unbox");
    RESOLVE_EXPORT_OPTIONAL(value_box, "il2cpp_value_box");
    RESOLVE_EXPORT_OPTIONAL(object_get_class, "il2cpp_object_get_class");
    RESOLVE_EXPORT_OPTIONAL(object_to_string, "il2cpp_object_to_string");
    RESOLVE_EXPORT_OPTIONAL(runtime_class_init, "il2cpp_runtime_class_init");
    RESOLVE_EXPORT_OPTIONAL(thread_current, "il2cpp_thread_current");

#undef RESOLVE_PATTERN
#undef RESOLVE_EXPORT
#undef RESOLVE_EXPORT_OPTIONAL

    void* ctor_scan = scan_module(config::kGameAssemblyModule,
        "\xF3\x0F\x11\x4C\x24\x00\x4C\x8B\xDC\x48\x81\xEC\x00\x00\x00\x00\x49\x8D\x43\x00\x49\x89\x4B",
        "xxxxx?xxxxxx????xxx?xxx");

    if (!ctor_scan) {
        ctor_scan = scan_module(config::kGameAssemblyModule,
            "\xF3\x0F\x11\x54\x24\x00\xF3\x0F\x11\x4C\x24\x00\x4C\x8B\xDC\x48\x83\xEC\x00\x49\x89\x4B\x00"
            "\x49\x8B\xC1\x49\x8D\x4B\x00\x49\xC7\x43\x00\x00\x00\x00\x00\x49\x89\x4B\x00\x4D\x8D\x4B\x00"
            "\x49\x8D\x4B\x00\x45\x33\xC0\x49\x89\x4B\x00\x48\x8B\xD0\x48\x8B\x48",
            "xxxxx?xxxxx?xxxxxx?xxx?xxxxxx?xxx?????xxx?xxx?xxx?xxxxxx?xxxxxx");
    }

    if (ctor_scan) {
        auto* mov = static_cast<std::uint8_t*>(
            scan(reinterpret_cast<std::uintptr_t>(ctor_scan),
                 reinterpret_cast<std::uintptr_t>(ctor_scan) + 0x7E,
                 "\x48\x8B\x48", "xxx"));
        if (mov) {
            c.method_pointer_offset = mov[3];
        }
    }
    // Offset 0 is valid - MethodInfo::methodPointer is the first field on most Unity builds.
    // The old check treated 0 as "missing" and failed every resolve.
    LOG_INFO("IL2CPP", "MethodInfo::methodPointer offset = 0x%X", c.method_pointer_offset);

    struct Required {
        const char* name;
        void* ptr;
    };
    const Required required[] = {
        { "il2cpp_domain_get", reinterpret_cast<void*>(c.api.domain_get) },
        { "il2cpp_thread_attach", reinterpret_cast<void*>(c.api.thread_attach) },
        { "il2cpp_domain_get_assemblies", reinterpret_cast<void*>(c.api.get_assemblies) },
        { "il2cpp_assembly_get_image", reinterpret_cast<void*>(c.api.get_image) },
        { "il2cpp_image_get_name", reinterpret_cast<void*>(c.api.get_image_name) },
        { "il2cpp_class_from_name", reinterpret_cast<void*>(c.api.get_class_by_name) },
        { "il2cpp_class_get_name", reinterpret_cast<void*>(c.api.get_class_name) },
        { "il2cpp_class_get_methods", reinterpret_cast<void*>(c.api.get_methods) },
        { "il2cpp_class_get_fields", reinterpret_cast<void*>(c.api.get_fields) },
        { "il2cpp_class_get_method_from_name", reinterpret_cast<void*>(c.api.get_method_by_name) },
        { "il2cpp_class_get_field_from_name", reinterpret_cast<void*>(c.api.get_field_by_name) },
    };

    bool ok = true;
    for (const auto& item : required) {
        if (!item.ptr) {
            LOG_ERROR("IL2CPP", "Missing %s", item.name);
            ok = false;
        }
    }

    if (!ok) {
        LOG_FATAL("IL2CPP", "API resolution incomplete - auto-scan could not find required exports");
    }

    return ok;
}

} // namespace il2cpp_base::il2cpp::detail
