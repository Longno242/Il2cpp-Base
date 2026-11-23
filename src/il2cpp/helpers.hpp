#pragma once

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <vector>

#include <Windows.h>

#include "il2cpp/resolver.hpp"

namespace il2cpp_base::il2cpp {

template<typename T>
[[nodiscard]] inline bool mem_read(const void* address, T& out) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (!address) {
        return false;
    }
    __try {
        std::memcpy(&out, address, sizeof(T));
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template<typename T>
[[nodiscard]] inline bool mem_write(void* address, const T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (!address) {
        return false;
    }
    __try {
        std::memcpy(address, &value, sizeof(T));
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

[[nodiscard]] inline std::string string_from_il2cpp(void* str) {
    if (!str) {
        return {};
    }
    auto& c = detail::ctx();
    if (!c.api.string_chars || !c.api.string_length) {
        return {};
    }
    const wchar_t* chars = detail::spoofer::call(c.jmp_rdx, c.api.string_chars, str);
    const int length = detail::spoofer::call(c.jmp_rdx, c.api.string_length, str);
    if (!chars || length <= 0) {
        return {};
    }
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, chars, length, nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, chars, length, result.data(), bytes, nullptr, nullptr);
    return result;
}

[[nodiscard]] inline void* string_to_il2cpp(const char* text) {
    if (!text) {
        return nullptr;
    }
    auto& c = detail::ctx();
    if (!c.api.string_new) {
        return nullptr;
    }
    return detail::spoofer::call(c.jmp_rdx, c.api.string_new, text);
}

template<typename T>
[[nodiscard]] inline T unbox(void* boxed) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (!boxed) {
        return {};
    }
    auto& c = detail::ctx();
    if (!c.api.object_unbox) {
        return {};
    }
    void* data = detail::spoofer::call(c.jmp_rdx, c.api.object_unbox, boxed);
    if (!data) {
        return {};
    }
    T value{};
    std::memcpy(&value, data, sizeof(T));
    return value;
}

[[nodiscard]] inline std::string object_class_name(void* object) {
    if (!object) {
        return {};
    }
    auto& c = detail::ctx();
    if (!c.api.object_get_class || !c.api.get_class_name) {
        return {};
    }
    void* klass = detail::spoofer::call(c.jmp_rdx, c.api.object_get_class, object);
    if (!klass) {
        return {};
    }
    const char* namespaze = c.api.get_class_namespace
        ? detail::spoofer::call(c.jmp_rdx, c.api.get_class_namespace, klass)
        : nullptr;
    const char* name = detail::spoofer::call(c.jmp_rdx, c.api.get_class_name, klass);
    if (!name) {
        return {};
    }
    if (namespaze && *namespaze) {
        return std::string(namespaze) + "." + name;
    }
    return name;
}

[[nodiscard]] inline std::vector<Method*> methods_named(Class* klass, const std::string& name) {
    std::vector<Method*> result;
    if (!klass) {
        return result;
    }
    for (Method* method : klass->methods()) {
        if (method->name() == name) {
            result.push_back(method);
        }
    }
    return result;
}

// Picks an overload by parameter types, e.g. method_with_params(k, "FindObjectsOfType", { "System.Type" }).
[[nodiscard]] inline Method* method_with_params(Class* klass, const std::string& name,
                                                std::initializer_list<const char*> param_types) {
    for (Method* method : methods_named(klass, name)) {
        if (method->param_count() != param_types.size()) {
            continue;
        }
        bool match = true;
        std::size_t i = 0;
        for (const char* type : param_types) {
            match = match && method->params()[i++].first == type;
        }
        if (match) {
            return method;
        }
    }
    return nullptr;
}

// Searches every loaded assembly. Prefer image(...)->klass(...) when you know the assembly.
[[nodiscard]] inline Class* search_class(const std::string& name, const std::string& namespaze = "") {
    for (Image* img : images()) {
        if (Class* klass = img->klass(name, namespaze)) {
            return klass;
        }
    }
    return nullptr;
}

[[nodiscard]] inline void* class_of(void* object) {
    auto& c = detail::ctx();
    return object ? detail::spoofer::call(c.jmp_rdx, c.api.object_get_class, object) : nullptr;
}

// The System.Type object for a class, for APIs like Object.FindObjectsOfType(Type).
[[nodiscard]] inline void* type_object(const Class* klass) {
    auto& c = detail::ctx();
    if (!klass || !c.api.class_get_type || !c.api.type_get_object) {
        return nullptr;
    }
    const void* type = detail::spoofer::call(c.jmp_rdx, c.api.class_get_type, const_cast<void*>(klass->raw()));
    return type ? detail::spoofer::call(c.jmp_rdx, c.api.type_get_object, type) : nullptr;
}

[[nodiscard]] inline bool is_instance_of(void* object, const Class* klass) {
    auto& c = detail::ctx();
    void* object_class = class_of(object);
    if (!object_class || !klass || !c.api.class_is_assignable_from) {
        return false;
    }
    return detail::spoofer::call(c.jmp_rdx, c.api.class_is_assignable_from,
                                 const_cast<void*>(klass->raw()), object_class);
}

// Managed arrays: Il2CppObject header, bounds pointer, length, then the elements.
inline constexpr std::size_t kArrayLengthOffset = 0x18;
inline constexpr std::size_t kArrayDataOffset = 0x20;

[[nodiscard]] inline std::size_t array_length(void* array) {
    if (!array) {
        return 0;
    }
    auto& c = detail::ctx();
    if (c.api.array_length) {
        return detail::spoofer::call(c.jmp_rdx, c.api.array_length, array);
    }
    return *reinterpret_cast<std::uintptr_t*>(static_cast<std::uint8_t*>(array) + kArrayLengthOffset);
}

template<typename T>
[[nodiscard]] inline T* array_data(void* array) {
    return array ? reinterpret_cast<T*>(static_cast<std::uint8_t*>(array) + kArrayDataOffset) : nullptr;
}

template<typename T>
[[nodiscard]] inline std::vector<T> array_to_vector(void* array) {
    const std::size_t length = array_length(array);
    const T* data = array_data<T>(array);
    return data ? std::vector<T>(data, data + length) : std::vector<T>{};
}

// A strong GC handle keeps an object alive (and its pointer valid) until released.
[[nodiscard]] inline std::uint32_t gc_hold(void* object) {
    auto& c = detail::ctx();
    return object && c.api.gchandle_new ? detail::spoofer::call(c.jmp_rdx, c.api.gchandle_new, object, false) : 0;
}

[[nodiscard]] inline void* gc_target(std::uint32_t handle) {
    auto& c = detail::ctx();
    return handle && c.api.gchandle_get_target ? detail::spoofer::call(c.jmp_rdx, c.api.gchandle_get_target, handle) : nullptr;
}

inline void gc_release(std::uint32_t handle) {
    auto& c = detail::ctx();
    if (handle && c.api.gchandle_free) {
        detail::spoofer::call(c.jmp_rdx, c.api.gchandle_free, handle);
    }
}

// Native function behind an [InternalCall], e.g. "UnityEngine.Time::get_deltaTime()".
[[nodiscard]] inline void* resolve_icall(const char* name) {
    auto& c = detail::ctx();
    return c.api.resolve_icall ? detail::spoofer::call(c.jmp_rdx, c.api.resolve_icall, name) : nullptr;
}

// Reads a property through its getter. Returns the object, or the boxed value for value types.
[[nodiscard]] inline void* get_property(void* object, Class* klass, const std::string& name) {
    Method* getter = klass ? klass->method("get_" + name, 0) : nullptr;
    return getter ? getter->invoke(object) : nullptr;
}

template<typename T>
inline bool set_property(void* object, Class* klass, const std::string& name, T value) {
    Method* setter = klass ? klass->method("set_" + name, 1) : nullptr;
    if (!setter) {
        return false;
    }
    void* args[] = { &value };
    setter->invoke(object, args);
    return true;
}

[[nodiscard]] inline std::string object_to_string(void* object) {
    if (!object) {
        return {};
    }
    auto& c = detail::ctx();
    static Method* to_string = [] {
        Class* object_class = search_class("Object", "System");
        return object_class ? object_class->method("ToString", 0) : nullptr;
    }();
    if (to_string && c.api.object_get_virtual_method && c.api.runtime_invoke) {
        const void* method = detail::spoofer::call(c.jmp_rdx, c.api.object_get_virtual_method, object, to_string->raw());
        void* exc = nullptr;
        void* str = method ? detail::spoofer::call(c.jmp_rdx, c.api.runtime_invoke, method, object,
                                                   static_cast<void**>(nullptr), &exc) : nullptr;
        if (str && !exc) {
            if (std::string text = string_from_il2cpp(str); !text.empty()) {
                return text;
            }
        }
    }
    return object_class_name(object);
}

} // namespace il2cpp_base::il2cpp
