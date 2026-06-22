#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "il2cpp_base/core/config.hpp"
#include "il2cpp_base/core/logger.hpp"
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

inline void* find_export_by_name(const std::string& name) {
    static std::unordered_map<std::string, void*> cache;
    static bool scanned = false;

    if (scanned) {
        const auto it = cache.find(name);
        return it != cache.end() ? it->second : nullptr;
    }

    void* start = scan_module(config::kUnityPlayerModule,
        "\x48\x89\x5C\x24\x00\x48\x8D\x15\x00\x00\x00\x00\xB3", "xxxx?xxx????x");
    void* end = scan_module(config::kUnityPlayerModule,
        "\xE8\x00\x00\x00\x00\x48\x8B\x5C\x24\x00\x32\xC0\x48\xC7\x05", "x????xxxx?xxxxx");

    if (!start || !end) {
        LOG_ERROR("IL2CPP", "Failed to locate UnityPlayer export table scan region");
        scanned = true;
        return nullptr;
    }

    void* first_match = nullptr;
    auto* cursor = static_cast<std::uint8_t*>(start);

    while (cursor < static_cast<std::uint8_t*>(end)) {
        auto* lea = static_cast<std::uint8_t*>(
            scan(reinterpret_cast<std::uintptr_t>(cursor),
                 reinterpret_cast<std::uintptr_t>(end),
                 "\x48\x8D\x15", "xxx"));
        if (!lea) {
            break;
        }

        const auto fn_name = std::string(reinterpret_cast<char*>(lea + *reinterpret_cast<std::int32_t*>(lea + 3) + 7));

        auto* mov = static_cast<std::uint8_t*>(
            scan(reinterpret_cast<std::uintptr_t>(lea + 1),
                 reinterpret_cast<std::uintptr_t>(end),
                 "\x48\x89\x05", "xxx"));
        if (!mov) {
            break;
        }

        void* fn_addr = *reinterpret_cast<void**>(mov + *reinterpret_cast<std::int32_t*>(mov + 3) + 7);
        cache[fn_name] = fn_addr;

        if (name == fn_name) {
            first_match = fn_addr;
        }

        cursor = mov + 1;
    }

    scanned = true;
    LOG_DEBUG("IL2CPP", "Scanned %zu il2cpp exports from UnityPlayer", cache.size());
    return first_match;
}

inline bool resolve_api() {
    auto& c = ctx();

#define RESOLVE_PATTERN(field, pattern, mask) \
    c.api.field = reinterpret_cast<decltype(c.api.field)>( \
        scan_module(config::kGameAssemblyModule, pattern, mask)); \
    if (!c.api.field) LOG_ERROR("IL2CPP", "Missing pattern: " #field)

#define RESOLVE_EXPORT(field, export_name) \
    c.api.field = reinterpret_cast<decltype(c.api.field)>(find_export_by_name(export_name)); \
    if (!c.api.field) LOG_ERROR("IL2CPP", "Missing export: " export_name)

    RESOLVE_PATTERN(domain_get,
        "\x48\x83\xEC\x00\x48\x63\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00"
        "\x4C\x8B\x05\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x8B\x0C\x08\x48\x8B\x44\x24\x00"
        "\x48\xC1\xE1\x00\x48\x03\xCA\x48\x3B\xC1\x73\x00\x48\x8B\x44\x24\x00\x48\x3B\xC2\x73\x00"
        "\x49\x63\x40\x00\x42\x8B\x4C\x00\x00\x48\x8B\x44\x24\x00\x49\x8D\x14\x88\x48\x3B\xC2\x0F\x83",
        "xxx?xxx????xxx????xxx????xxx????xxxxxxx?xxx?xxxxxxx?xxxx?xxxx?xxx?xxxx?xxxx?xxxxxxxxx");

    RESOLVE_PATTERN(thread_attach,
        "\x40\x56\x48\x83\xEC\x00\x48\x8B\xF1\x48\x8B\x0D\x00\x00\x00\x00\x8B\x09",
        "xxxxx?xxxxxx????xx");

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

#undef RESOLVE_PATTERN
#undef RESOLVE_EXPORT

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
            LOG_DEBUG("IL2CPP", "MethodInfo::methodPointer offset = 0x%X", c.method_pointer_offset);
        }
    } else {
        LOG_ERROR("IL2CPP", "Failed to resolve MethodInfo::methodPointer offset");
    }

    const bool ok =
        c.api.domain_get && c.api.thread_attach && c.api.get_assemblies && c.api.get_image &&
        c.api.get_image_name && c.api.get_class_count && c.api.get_class_by_name && c.api.get_class &&
        c.api.get_class_name && c.api.get_class_namespace && c.api.is_class_enum &&
        c.api.is_class_valuetype && c.api.get_fields && c.api.get_field_by_name &&
        c.api.get_field_name && c.api.get_field_flags && c.api.get_field_offset &&
        c.api.get_field_type && c.api.get_type_name && c.api.get_method_by_name &&
        c.api.get_methods && c.api.get_method_name && c.api.get_method_return_type &&
        c.api.get_method_param_count && c.api.get_method_param_name && c.api.is_class_generic &&
        c.api.get_method_param_type && c.api.free_memory && c.api.get_method_flags &&
        c.api.is_method_instance && c.api.get_class_nested_types && c.method_pointer_offset;

    if (!ok) {
        LOG_FATAL("IL2CPP", "API resolution incomplete — patterns may need updating for this game build");
    }

    return ok;
}

} // namespace il2cpp_base::il2cpp::detail
