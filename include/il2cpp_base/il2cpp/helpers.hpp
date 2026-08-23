#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include <Windows.h>

#include "il2cpp_base/il2cpp/resolver.hpp"

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

[[nodiscard]] inline std::string object_to_string(void* object) {
    if (!object) {
        return {};
    }
    auto& c = detail::ctx();
    if (c.api.object_to_string) {
        void* exc = nullptr;
        void* str = detail::spoofer::call(c.jmp_rdx, c.api.object_to_string, object, &exc);
        if (str && !exc) {
            if (std::string text = string_from_il2cpp(str); !text.empty()) {
                return text;
            }
        }
    }
    return object_class_name(object);
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

} // namespace il2cpp_base::il2cpp
