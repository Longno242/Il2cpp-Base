#pragma once

#include <cstdint>
#include <cstring>

#include <Windows.h>

namespace il2cpp_base::il2cpp::detail {

inline std::uintptr_t module_base(const char* module_name) {
    const HMODULE handle = GetModuleHandleA(module_name);
    return handle ? reinterpret_cast<std::uintptr_t>(handle) : 0;
}

inline std::uintptr_t to_rva(std::uintptr_t address, std::uintptr_t base) {
    return address > base ? address - base : address;
}

inline void* scan(std::uintptr_t start, std::uintptr_t end, const char* pattern, const char* mask) {
    if (!pattern || !mask) {
        return nullptr;
    }

    const std::size_t len = std::strlen(mask);
    if (end <= start || len > (end - start)) {
        return nullptr;
    }

    auto* begin = reinterpret_cast<std::uint8_t*>(start);
    auto* limit = reinterpret_cast<std::uint8_t*>(end) - len;

    for (auto* curr = begin; curr <= limit; ++curr) {
        bool match = true;
        for (std::size_t i = 0; i < len; ++i) {
            if (mask[i] != '?' && static_cast<std::uint8_t>(pattern[i]) != curr[i]) {
                match = false;
                break;
            }
        }
        if (match) {
            return curr;
        }
    }

    return nullptr;
}

inline void* scan_module(const char* module_name, const char* pattern, const char* mask) {
    const auto base = module_base(module_name);
    if (!base) {
        return nullptr;
    }

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    return scan(base, base + nt->OptionalHeader.SizeOfImage, pattern, mask);
}

inline std::uint64_t fnv1a_class(const char* namespaze, const char* name) {
    std::uint64_t hash = 0x14650FB0739D0383ULL;

    while (namespaze && *namespaze) {
        hash ^= static_cast<std::uint8_t>(*namespaze++);
        hash *= 0x100000001B3ULL;
    }

    hash ^= ':';
    hash *= 0x100000001B3ULL;

    while (name && *name) {
        hash ^= static_cast<std::uint8_t>(*name++);
        hash *= 0x100000001B3ULL;
    }

    return hash;
}

} // namespace il2cpp_base::il2cpp::detail
