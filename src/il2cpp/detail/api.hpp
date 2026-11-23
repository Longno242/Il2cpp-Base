#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "config.hpp"
#include "core/logger.hpp"
#include "il2cpp/detail/exports.hpp"
#include "il2cpp/detail/scan.hpp"
#include "il2cpp/detail/spoofer.hpp"
#include "il2cpp/types.hpp"

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
    void (*runtime_class_init)(void* klass) = nullptr;
    void* (*thread_current)() = nullptr;

    // Classes and types
    void* (*class_get_parent)(void* klass) = nullptr;
    const void* (*class_get_type)(void* klass) = nullptr;
    void* (*class_from_type)(const void* type) = nullptr;
    void* (*class_from_system_type)(void* reflection_type) = nullptr;
    const void* (*class_get_image)(void* klass) = nullptr;
    void* (*class_get_element_class)(void* klass) = nullptr;
    void* (*class_get_declaring_type)(void* klass) = nullptr;
    void* (*class_get_interfaces)(void* klass, void** iter) = nullptr;
    std::int32_t (*class_instance_size)(void* klass) = nullptr;
    std::int32_t (*class_array_element_size)(const void* klass) = nullptr;
    int (*class_get_flags)(const void* klass) = nullptr;
    int (*class_get_rank)(const void* klass) = nullptr;
    bool (*class_is_abstract)(const void* klass) = nullptr;
    bool (*class_is_interface)(const void* klass) = nullptr;
    bool (*class_is_subclass_of)(void* klass, void* parent, bool check_interfaces) = nullptr;
    bool (*class_is_assignable_from)(void* klass, void* other) = nullptr;
    bool (*class_has_parent)(void* klass, void* parent) = nullptr;
    const char* (*class_get_assemblyname)(const void* klass) = nullptr;
    void* (*type_get_object)(const void* type) = nullptr;
    int (*type_get_type)(const void* type) = nullptr;

    // Properties
    const void* (*class_get_properties)(void* klass, void** iter) = nullptr;
    const void* (*class_get_property_from_name)(void* klass, const char* name) = nullptr;
    const char* (*property_get_name)(const void* prop) = nullptr;
    const void* (*property_get_get_method)(const void* prop) = nullptr;
    const void* (*property_get_set_method)(const void* prop) = nullptr;

    // Methods and fields
    void* (*method_get_class)(const void* method) = nullptr;
    void* (*method_get_declaring_type)(const void* method) = nullptr;
    const void* (*object_get_virtual_method)(void* obj, const void* method) = nullptr;
    void (*field_get_value)(void* obj, void* field, void* value) = nullptr;
    void (*field_set_value)(void* obj, void* field, void* value) = nullptr;
    void* (*field_get_value_object)(void* field, void* obj) = nullptr;
    void* (*field_get_parent)(void* field) = nullptr;

    // Objects, strings and arrays
    std::uint32_t (*object_get_size)(void* obj) = nullptr;
    void* (*string_new_utf16)(const wchar_t* text, std::int32_t length) = nullptr;
    void* (*array_new)(void* element_class, std::uintptr_t length) = nullptr;
    std::uint32_t (*array_length)(void* array) = nullptr;
    void* (*array_class_get)(void* element_class, std::uint32_t rank) = nullptr;

    // GC handles keep managed objects alive between frames
    std::uint32_t (*gchandle_new)(void* obj, bool pinned) = nullptr;
    std::uint32_t (*gchandle_new_weakref)(void* obj, bool track_resurrection) = nullptr;
    void* (*gchandle_get_target)(std::uint32_t handle) = nullptr;
    void (*gchandle_free)(std::uint32_t handle) = nullptr;
    void (*gc_collect)(int max_generation) = nullptr;
    std::int64_t (*gc_get_used_size)() = nullptr;
    std::int64_t (*gc_get_heap_size)() = nullptr;

    // Runtime
    void* (*resolve_icall)(const char* name) = nullptr;
    const void* (*get_corlib)() = nullptr;
    const void* (*domain_assembly_open)(void* domain, const char* name) = nullptr;
    const void* (*image_get_assembly)(const void* image) = nullptr;
    const char* (*image_get_filename)(const void* image) = nullptr;
    void (*thread_detach)(void* thread) = nullptr;
    bool (*is_vm_thread)(void* thread) = nullptr;
    void* (*exception_from_name_msg)(const void* image, const char* namespaze, const char* name, const char* msg) = nullptr;
    void (*format_exception)(const void* exc, char* message, int size) = nullptr;
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
    RESOLVE_EXPORT_OPTIONAL(runtime_class_init, "il2cpp_runtime_class_init");
    RESOLVE_EXPORT_OPTIONAL(thread_current, "il2cpp_thread_current");

    RESOLVE_EXPORT_OPTIONAL(class_get_parent, "il2cpp_class_get_parent");
    RESOLVE_EXPORT_OPTIONAL(class_get_type, "il2cpp_class_get_type");
    RESOLVE_EXPORT_OPTIONAL(class_from_type, "il2cpp_class_from_type");
    RESOLVE_EXPORT_OPTIONAL(class_from_system_type, "il2cpp_class_from_system_type");
    RESOLVE_EXPORT_OPTIONAL(class_get_image, "il2cpp_class_get_image");
    RESOLVE_EXPORT_OPTIONAL(class_get_element_class, "il2cpp_class_get_element_class");
    RESOLVE_EXPORT_OPTIONAL(class_get_declaring_type, "il2cpp_class_get_declaring_type");
    RESOLVE_EXPORT_OPTIONAL(class_get_interfaces, "il2cpp_class_get_interfaces");
    RESOLVE_EXPORT_OPTIONAL(class_instance_size, "il2cpp_class_instance_size");
    RESOLVE_EXPORT_OPTIONAL(class_array_element_size, "il2cpp_class_array_element_size");
    RESOLVE_EXPORT_OPTIONAL(class_get_flags, "il2cpp_class_get_flags");
    RESOLVE_EXPORT_OPTIONAL(class_get_rank, "il2cpp_class_get_rank");
    RESOLVE_EXPORT_OPTIONAL(class_is_abstract, "il2cpp_class_is_abstract");
    RESOLVE_EXPORT_OPTIONAL(class_is_interface, "il2cpp_class_is_interface");
    RESOLVE_EXPORT_OPTIONAL(class_is_subclass_of, "il2cpp_class_is_subclass_of");
    RESOLVE_EXPORT_OPTIONAL(class_is_assignable_from, "il2cpp_class_is_assignable_from");
    RESOLVE_EXPORT_OPTIONAL(class_has_parent, "il2cpp_class_has_parent");
    RESOLVE_EXPORT_OPTIONAL(class_get_assemblyname, "il2cpp_class_get_assemblyname");
    RESOLVE_EXPORT_OPTIONAL(type_get_object, "il2cpp_type_get_object");
    RESOLVE_EXPORT_OPTIONAL(type_get_type, "il2cpp_type_get_type");

    RESOLVE_EXPORT_OPTIONAL(class_get_properties, "il2cpp_class_get_properties");
    RESOLVE_EXPORT_OPTIONAL(class_get_property_from_name, "il2cpp_class_get_property_from_name");
    RESOLVE_EXPORT_OPTIONAL(property_get_name, "il2cpp_property_get_name");
    RESOLVE_EXPORT_OPTIONAL(property_get_get_method, "il2cpp_property_get_get_method");
    RESOLVE_EXPORT_OPTIONAL(property_get_set_method, "il2cpp_property_get_set_method");

    RESOLVE_EXPORT_OPTIONAL(method_get_class, "il2cpp_method_get_class");
    RESOLVE_EXPORT_OPTIONAL(method_get_declaring_type, "il2cpp_method_get_declaring_type");
    RESOLVE_EXPORT_OPTIONAL(object_get_virtual_method, "il2cpp_object_get_virtual_method");
    RESOLVE_EXPORT_OPTIONAL(field_get_value, "il2cpp_field_get_value");
    RESOLVE_EXPORT_OPTIONAL(field_set_value, "il2cpp_field_set_value");
    RESOLVE_EXPORT_OPTIONAL(field_get_value_object, "il2cpp_field_get_value_object");
    RESOLVE_EXPORT_OPTIONAL(field_get_parent, "il2cpp_field_get_parent");

    RESOLVE_EXPORT_OPTIONAL(object_get_size, "il2cpp_object_get_size");
    RESOLVE_EXPORT_OPTIONAL(string_new_utf16, "il2cpp_string_new_utf16");
    RESOLVE_EXPORT_OPTIONAL(array_new, "il2cpp_array_new");
    RESOLVE_EXPORT_OPTIONAL(array_length, "il2cpp_array_length");
    RESOLVE_EXPORT_OPTIONAL(array_class_get, "il2cpp_array_class_get");

    RESOLVE_EXPORT_OPTIONAL(gchandle_new, "il2cpp_gchandle_new");
    RESOLVE_EXPORT_OPTIONAL(gchandle_new_weakref, "il2cpp_gchandle_new_weakref");
    RESOLVE_EXPORT_OPTIONAL(gchandle_get_target, "il2cpp_gchandle_get_target");
    RESOLVE_EXPORT_OPTIONAL(gchandle_free, "il2cpp_gchandle_free");
    RESOLVE_EXPORT_OPTIONAL(gc_collect, "il2cpp_gc_collect");
    RESOLVE_EXPORT_OPTIONAL(gc_get_used_size, "il2cpp_gc_get_used_size");
    RESOLVE_EXPORT_OPTIONAL(gc_get_heap_size, "il2cpp_gc_get_heap_size");

    RESOLVE_EXPORT_OPTIONAL(resolve_icall, "il2cpp_resolve_icall");
    RESOLVE_EXPORT_OPTIONAL(get_corlib, "il2cpp_get_corlib");
    RESOLVE_EXPORT_OPTIONAL(domain_assembly_open, "il2cpp_domain_assembly_open");
    RESOLVE_EXPORT_OPTIONAL(image_get_assembly, "il2cpp_image_get_assembly");
    RESOLVE_EXPORT_OPTIONAL(image_get_filename, "il2cpp_image_get_filename");
    RESOLVE_EXPORT_OPTIONAL(thread_detach, "il2cpp_thread_detach");
    RESOLVE_EXPORT_OPTIONAL(is_vm_thread, "il2cpp_is_vm_thread");
    RESOLVE_EXPORT_OPTIONAL(exception_from_name_msg, "il2cpp_exception_from_name_msg");
    RESOLVE_EXPORT_OPTIONAL(format_exception, "il2cpp_format_exception");

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
    // Offset 0 is valid: MethodInfo::methodPointer is the first field on most Unity builds.
    LOG_INFO("IL2CPP", "MethodInfo::methodPointer offset = 0x%X", c.method_pointer_offset);

    const auto* slots = reinterpret_cast<void* const*>(&c.api);
    constexpr std::size_t slot_count = sizeof(ApiTable) / sizeof(void*);
    const auto resolved = static_cast<std::size_t>(std::count_if(slots, slots + slot_count, [](void* p) { return p != nullptr; }));
    LOG_INFO("IL2CPP", "resolved %zu/%zu exports", resolved, slot_count);

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
