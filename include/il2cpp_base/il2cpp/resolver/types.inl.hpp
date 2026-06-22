#pragma once

#include <algorithm>
#include <cstdio>

#include "il2cpp_base/core/config.hpp"
#include "il2cpp_base/il2cpp/resolver/detail/api.hpp"
#include "il2cpp_base/il2cpp/resolver/detail/scan.hpp"
#include "il2cpp_base/il2cpp/resolver/detail/spoofer.hpp"
#include "il2cpp_base/il2cpp/resolver/flags.hpp"
#include "il2cpp_base/il2cpp/resolver/types.hpp"

namespace il2cpp_base::il2cpp {

// ---------------------------------------------------------------------------
// Field
// ---------------------------------------------------------------------------
inline Field::Field(std::string name, std::string type, std::size_t offset,
                    std::uint32_t flags, const void* field)
    : name_(std::move(name))
    , type_(std::move(type))
    , offset_(offset)
    , flags_(flags)
    , field_(field) {
    const auto base = detail::module_base(config::kGameAssemblyModule);
    if (is_static() && field_) {
        static_rva_ = detail::to_rva(reinterpret_cast<std::uintptr_t>(field_), base);
    }
}

inline std::string Field::describe() const {
    char buf[128]{};
    if (is_static()) {
        snprintf(buf, sizeof(buf), "static RVA 0x%llX", static_cast<unsigned long long>(static_rva_));
    } else {
        snprintf(buf, sizeof(buf), "instance +0x%llX", static_cast<unsigned long long>(offset_));
    }
    return buf;
}

// ---------------------------------------------------------------------------
// Method
// ---------------------------------------------------------------------------
inline Method::Method(std::string name, std::string return_type,
                      std::vector<std::pair<std::string, std::string>> params,
                      std::uint32_t flags, void* address, const void* method_info)
    : name_(std::move(name))
    , return_type_(std::move(return_type))
    , flags_(flags)
    , params_(std::move(params))
    , address_(address)
    , method_info_(method_info) {
    const auto base = detail::module_base(config::kGameAssemblyModule);
    if (address_) {
        rva_ = detail::to_rva(reinterpret_cast<std::uintptr_t>(address_), base);
    }
}

inline std::string Method::describe() const {
    char buf[160]{};
    snprintf(buf, sizeof(buf), "%s -> %s @ 0x%llX",
             name_.c_str(), return_type_.c_str(),
             static_cast<unsigned long long>(rva_));
    return buf;
}

// ---------------------------------------------------------------------------
// Class
// ---------------------------------------------------------------------------
inline Class::Class(std::string name, std::string namespaze, const void* klass)
    : name_(std::move(name))
    , namespaze_(std::move(namespaze))
    , klass_(klass)
    , module_base_(detail::module_base(config::kGameAssemblyModule)) {}

inline std::uintptr_t Class::rva() const {
    return detail::to_rva(reinterpret_cast<std::uintptr_t>(klass_), module_base_);
}

inline bool Class::is_enum() {
    auto& c = detail::ctx();
    return detail::spoofer::call(c.jmp_rdx, c.api.is_class_enum, klass_);
}

inline bool Class::is_generic() {
    auto& c = detail::ctx();
    return detail::spoofer::call(c.jmp_rdx, c.api.is_class_generic, klass_);
}

inline bool Class::is_value_type() {
    auto& c = detail::ctx();
    return detail::spoofer::call(c.jmp_rdx, c.api.is_class_valuetype, klass_);
}

inline std::string Class::type_name(const void* type) {
    auto& c = detail::ctx();
    char* name = detail::spoofer::call(c.jmp_rdx, c.api.get_type_name, type);
    if (!name || !*name) {
        return "unknown";
    }
    std::string result = name;
    detail::spoofer::call(c.jmp_rdx, c.api.free_memory, static_cast<void*>(name));
    return result;
}

inline Field* Class::field(const std::string& name) {
    const std::uint64_t key = detail::fnv1a_class(namespaze_.c_str(), name_.c_str())
                            ^ detail::fnv1a_class("", name.c_str());
    if (const auto it = field_cache_.find(key); it != field_cache_.end()) {
        return it->second;
    }

    auto& c = detail::ctx();
    void* raw = detail::spoofer::call(c.jmp_rdx, c.api.get_field_by_name,
                                      const_cast<void*>(klass_), name.c_str());
    if (!raw) {
        return nullptr;
    }

    const std::uint32_t flags = detail::spoofer::call(c.jmp_rdx, c.api.get_field_flags, raw);
    const void* ftype = detail::spoofer::call(c.jmp_rdx, c.api.get_field_type, raw);
    std::size_t offset = detail::spoofer::call(c.jmp_rdx, c.api.get_field_offset, raw);
    if (offset == static_cast<std::size_t>(-1)) {
        offset = 0;
    }

    auto owned = std::make_unique<Field>(name, type_name(ftype), offset, flags, raw);
    auto* ptr = owned.get();
    field_cache_[key] = ptr;
    owned_fields_.push_back(std::move(owned));
    return ptr;
}

inline const std::vector<Field*>& Class::fields() {
    if (!fields_ready_) {
        auto& c = detail::ctx();
        void* iter = nullptr;

        while (void* raw = detail::spoofer::call(c.jmp_rdx, c.api.get_fields,
                                                 const_cast<void*>(klass_), &iter)) {
            const char* fname = detail::spoofer::call(c.jmp_rdx, c.api.get_field_name, raw);
            if (!fname || !*fname) {
                continue;
            }

            const std::uint64_t key = detail::fnv1a_class(namespaze_.c_str(), name_.c_str())
                                    ^ detail::fnv1a_class("", fname);
            if (field_cache_.contains(key)) {
                continue;
            }

            const std::uint32_t flags = detail::spoofer::call(c.jmp_rdx, c.api.get_field_flags, raw);
            const void* ftype = detail::spoofer::call(c.jmp_rdx, c.api.get_field_type, raw);
            std::size_t offset = detail::spoofer::call(c.jmp_rdx, c.api.get_field_offset, raw);
            if (offset == static_cast<std::size_t>(-1)) {
                offset = 0;
            }

            auto owned = std::make_unique<Field>(fname, type_name(ftype), offset, flags, raw);
            field_cache_[key] = owned.get();
            owned_fields_.push_back(std::move(owned));
        }

        field_ptrs_.clear();
        field_ptrs_.reserve(owned_fields_.size());
        for (auto& f : owned_fields_) {
            field_ptrs_.push_back(f.get());
        }
        fields_ready_ = true;
    }
    return field_ptrs_;
}

inline Method* Class::method(const std::string& name, int arg_count) {
    auto& c = detail::ctx();
    const void* raw = detail::spoofer::call(c.jmp_rdx, c.api.get_method_by_name,
                                            const_cast<void*>(klass_), name.c_str(), arg_count);
    if (!raw) {
        return nullptr;
    }

    if (const auto it = std::find_if(method_ptrs_.begin(), method_ptrs_.end(),
            [raw](Method* m) { return m->raw() == raw; });
        it != method_ptrs_.end()) {
        return *it;
    }

    std::uint32_t iflags = 0;
    const std::uint32_t flags = detail::spoofer::call(c.jmp_rdx, c.api.get_method_flags, raw, &iflags);
    const void* ret_type = detail::spoofer::call(c.jmp_rdx, c.api.get_method_return_type, raw);
    void* addr = *reinterpret_cast<void**>(reinterpret_cast<std::uintptr_t>(raw) + c.method_pointer_offset);

    std::vector<std::pair<std::string, std::string>> params;
    const int count = arg_count >= 0 ? arg_count
        : static_cast<int>(detail::spoofer::call(c.jmp_rdx, c.api.get_method_param_count, raw));
    params.reserve(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i) {
        const void* ptype = detail::spoofer::call(c.jmp_rdx, c.api.get_method_param_type, raw,
                                                  static_cast<std::uint32_t>(i));
        const char* pname = detail::spoofer::call(c.jmp_rdx, c.api.get_method_param_name, raw,
                                                  static_cast<std::uint32_t>(i));
        if (ptype && pname) {
            params.emplace_back(type_name(ptype), pname);
        }
    }

    auto owned = std::make_unique<Method>(name, type_name(ret_type), std::move(params),
                                          flags, addr, raw);
    auto* ptr = owned.get();
    owned_methods_.push_back(std::move(owned));
    method_ptrs_.push_back(ptr);
    return ptr;
}

inline const std::vector<Method*>& Class::methods() {
    if (!methods_ready_) {
        auto& c = detail::ctx();
        void* iter = nullptr;

        while (const void* raw = detail::spoofer::call(c.jmp_rdx, c.api.get_methods,
                                                       const_cast<void*>(klass_), &iter)) {
            if (std::any_of(method_ptrs_.begin(), method_ptrs_.end(),
                    [raw](Method* m) { return m->raw() == raw; })) {
                continue;
            }

            const char* mname = detail::spoofer::call(c.jmp_rdx, c.api.get_method_name, raw);
            if (!mname || !*mname) {
                continue;
            }

            std::uint32_t iflags = 0;
            const std::uint32_t flags = detail::spoofer::call(c.jmp_rdx, c.api.get_method_flags, raw, &iflags);
            const void* ret_type = detail::spoofer::call(c.jmp_rdx, c.api.get_method_return_type, raw);
            void* addr = *reinterpret_cast<void**>(
                reinterpret_cast<std::uintptr_t>(raw) + c.method_pointer_offset);

            const std::uint32_t pcount = detail::spoofer::call(c.jmp_rdx, c.api.get_method_param_count, raw);
            std::vector<std::pair<std::string, std::string>> params;
            params.reserve(pcount);

            for (std::uint32_t i = 0; i < pcount; ++i) {
                const void* ptype = detail::spoofer::call(c.jmp_rdx, c.api.get_method_param_type, raw, i);
                const char* pname = detail::spoofer::call(c.jmp_rdx, c.api.get_method_param_name, raw, i);
                if (ptype && pname) {
                    params.emplace_back(type_name(ptype), pname);
                }
            }

            auto owned = std::make_unique<Method>(mname, type_name(ret_type), std::move(params),
                                                  flags, addr, raw);
            method_ptrs_.push_back(owned.get());
            owned_methods_.push_back(std::move(owned));
        }

        methods_ready_ = true;
    }
    return method_ptrs_;
}

inline Class* Class::nested(const std::string& name) {
    for (Class* cls : nested_types()) {
        if (cls->name() == name) {
            return cls;
        }
    }
    return nullptr;
}

inline const std::vector<Class*>& Class::nested_types() {
    if (!nested_ready_) {
        auto& c = detail::ctx();
        void* iter = nullptr;

        while (void* raw = detail::spoofer::call(c.jmp_rdx, c.api.get_class_nested_types,
                                                 const_cast<void*>(klass_), &iter)) {
            const char* n = detail::spoofer::call(c.jmp_rdx, c.api.get_class_name, raw);
            const char* ns = detail::spoofer::call(c.jmp_rdx, c.api.get_class_namespace, raw);
            if (!n || !ns) {
                continue;
            }

            auto owned = std::make_unique<Class>(n, ns, raw);
            nested_ptrs_.push_back(owned.get());
            owned_nested_.push_back(std::move(owned));
        }

        nested_ready_ = true;
    }
    return nested_ptrs_;
}

// ---------------------------------------------------------------------------
// Image
// ---------------------------------------------------------------------------
inline Image::Image(std::string name, const void* image)
    : name_(std::move(name))
    , image_(image)
    , module_base_(detail::module_base(config::kGameAssemblyModule)) {}

inline std::uintptr_t Image::rva() const {
    return detail::to_rva(reinterpret_cast<std::uintptr_t>(image_), module_base_);
}

inline Class* Image::klass(const std::string& name, const std::string& namespaze) {
    const std::uint64_t key = detail::fnv1a_class(namespaze.c_str(), name.c_str());
    if (const auto it = class_cache_.find(key); it != class_cache_.end()) {
        return it->second;
    }

    auto& c = detail::ctx();
    void* raw = detail::spoofer::call(c.jmp_rdx, c.api.get_class_by_name,
                                      image_, namespaze.c_str(), name.c_str());
    if (!raw) {
        return nullptr;
    }

    auto owned = std::make_unique<Class>(name, namespaze, raw);
    auto* ptr = owned.get();
    class_cache_[key] = ptr;
    owned_classes_.push_back(std::move(owned));
    class_ptrs_.push_back(ptr);
    return ptr;
}

inline const std::vector<Class*>& Image::classes() {
    if (!classes_ready_) {
        auto& c = detail::ctx();
        const std::size_t count = detail::spoofer::call(c.jmp_rdx, c.api.get_class_count, image_);
        owned_classes_.reserve(count);
        class_cache_.reserve(count);

        for (std::size_t i = 0; i < count; ++i) {
            const void* raw = detail::spoofer::call(c.jmp_rdx, c.api.get_class, image_, i);
            if (!raw) {
                continue;
            }

            const char* n = detail::spoofer::call(c.jmp_rdx, c.api.get_class_name, const_cast<void*>(raw));
            const char* ns = detail::spoofer::call(c.jmp_rdx, c.api.get_class_namespace, const_cast<void*>(raw));
            if (!n || !ns) {
                continue;
            }

            const std::uint64_t key = detail::fnv1a_class(ns, n);
            if (class_cache_.contains(key)) {
                continue;
            }

            auto owned = std::make_unique<Class>(n, ns, raw);
            class_cache_[key] = owned.get();
            owned_classes_.push_back(std::move(owned));
        }

        class_ptrs_.clear();
        class_ptrs_.reserve(owned_classes_.size());
        for (auto& cls : owned_classes_) {
            class_ptrs_.push_back(cls.get());
        }
        classes_ready_ = true;
    }
    return class_ptrs_;
}

} // namespace il2cpp_base::il2cpp
