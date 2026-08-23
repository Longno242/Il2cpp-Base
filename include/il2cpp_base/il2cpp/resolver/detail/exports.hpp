#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include <Windows.h>

#include "il2cpp_base/core/config.hpp"
#include "il2cpp_base/core/logger.hpp"
#include "il2cpp_base/il2cpp/resolver/detail/scan.hpp"

namespace il2cpp_base::il2cpp::detail {

inline bool safe_readable(const void* address, std::size_t size) {
    if (!address || size == 0) {
        return false;
    }
    __try {
        volatile std::uint8_t sum = 0;
        const auto* p = static_cast<const std::uint8_t*>(address);
        sum = static_cast<std::uint8_t>(p[0] + p[size - 1]);
        (void)sum;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

inline bool is_il2cpp_export_name(const char* text) {
    if (!text || !safe_readable(text, 8)) {
        return false;
    }
    if (std::memcmp(text, "il2cpp_", 7) != 0) {
        return false;
    }
    std::size_t n = 7;
    while (n < 96) {
        if (!safe_readable(text + n, 1)) {
            return false;
        }
        const char ch = text[n];
        if (ch == '\0') {
            return n > 7;
        }
        const bool ok =
            (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_';
        if (!ok) {
            return false;
        }
        ++n;
    }
    return false;
}

struct ModuleView {
    const char* name = nullptr;
    std::uintptr_t base = 0;
    std::uintptr_t image_end = 0;
    std::uintptr_t exec_start = 0;
    std::uintptr_t exec_end = 0;
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> data; // read, not execute
};

inline bool fill_module_view(HMODULE module, ModuleView& view) {
    if (!module) {
        return false;
    }
    view.base = reinterpret_cast<std::uintptr_t>(module);
    if (!safe_readable(module, sizeof(IMAGE_DOS_HEADER))) {
        return false;
    }
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(view.base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(view.base + dos->e_lfanew);
    if (!safe_readable(nt, sizeof(IMAGE_NT_HEADERS)) || nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    view.image_end = view.base + nt->OptionalHeader.SizeOfImage;
    const auto* section = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        const auto start = view.base + section[i].VirtualAddress;
        const auto size = section[i].Misc.VirtualSize
            ? section[i].Misc.VirtualSize
            : section[i].SizeOfRawData;
        if (!size) {
            continue;
        }
        const auto end = start + size;
        const bool readable = (section[i].Characteristics & IMAGE_SCN_MEM_READ) != 0;
        const bool executable = (section[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        if (readable && !executable) {
            view.data.emplace_back(start, end);
        }
        if (executable) {
            if (!view.exec_start) {
                view.exec_start = start;
                view.exec_end = end;
            } else {
                if (start < view.exec_start) {
                    view.exec_start = start;
                }
                if (end > view.exec_end) {
                    view.exec_end = end;
                }
            }
        }
    }
    return view.image_end > view.base;
}

inline bool in_range(std::uintptr_t address, std::uintptr_t start, std::uintptr_t end) {
    return address >= start && address < end;
}

inline bool in_module(const ModuleView& view, std::uintptr_t address) {
    return in_range(address, view.base, view.image_end);
}

inline bool in_exec(const ModuleView& view, std::uintptr_t address) {
    return view.exec_start && in_range(address, view.exec_start, view.exec_end);
}

inline void remember_export(std::unordered_map<std::string, void*>& cache,
                            const char* name, void* address) {
    if (!name || !address || !is_il2cpp_export_name(name)) {
        return;
    }
    cache.emplace(name, address);
}

inline void collect_pe_exports(HMODULE module, std::unordered_map<std::string, void*>& cache) {
    if (!module) {
        return;
    }
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (!safe_readable(dos, sizeof(IMAGE_DOS_HEADER)) || dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!dir.VirtualAddress || !dir.Size) {
        return;
    }

    const auto* exports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base + dir.VirtualAddress);
    if (!safe_readable(exports, sizeof(IMAGE_EXPORT_DIRECTORY))) {
        return;
    }

    const auto* names = reinterpret_cast<const DWORD*>(base + exports->AddressOfNames);
    const auto* ords  = reinterpret_cast<const WORD*>(base + exports->AddressOfNameOrdinals);
    const auto* funcs = reinterpret_cast<const DWORD*>(base + exports->AddressOfFunctions);
    if (!safe_readable(names, exports->NumberOfNames * sizeof(DWORD))) {
        return;
    }

    for (DWORD i = 0; i < exports->NumberOfNames; ++i) {
        const char* name = reinterpret_cast<const char*>(base + names[i]);
        if (!is_il2cpp_export_name(name)) {
            continue;
        }
        const DWORD rva = funcs[ords[i]];
        if (!rva) {
            continue;
        }
        // skip forwarded exports (rva lands inside the export directory)
        if (rva >= dir.VirtualAddress && rva < dir.VirtualAddress + dir.Size) {
            continue;
        }
        remember_export(cache, name, reinterpret_cast<void*>(base + rva));
    }
}

inline void collect_il2cpp_strings(const ModuleView& view,
                                   std::unordered_map<std::uintptr_t, const char*>& strings) {
    for (const auto& [start, end] : view.data) {
        if (end <= start + 8) {
            continue;
        }
        __try {
            auto* p = reinterpret_cast<const char*>(start);
            auto* last = reinterpret_cast<const char*>(end - 8);
            for (; p < last; ++p) {
                if (p[0] != 'i' || std::memcmp(p, "il2cpp_", 7) != 0) {
                    continue;
                }
                if (is_il2cpp_export_name(p)) {
                    strings.emplace(reinterpret_cast<std::uintptr_t>(p), p);
                    p += 6;
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
}

inline void collect_pointer_tables(const ModuleView& data_view,
                                   const std::vector<ModuleView>& code_views,
                                   const std::unordered_map<std::uintptr_t, const char*>& strings,
                                   std::unordered_map<std::string, void*>& cache) {
    if (strings.empty()) {
        return;
    }

    auto is_code = [&](std::uintptr_t address) {
        for (const auto& view : code_views) {
            if (in_exec(view, address)) {
                return true;
            }
        }
        return false;
    };

    for (const auto& [start, end] : data_view.data) {
        if (end < start + 16) {
            continue;
        }
        __try {
            const auto aligned = (start + 7) & ~static_cast<std::uintptr_t>(7);
            for (auto addr = aligned; addr + 16 <= end; addr += 8) {
                const auto* pair = reinterpret_cast<const std::uintptr_t*>(addr);
                const auto a = pair[0];
                const auto b = pair[1];

                if (const auto it = strings.find(a); it != strings.end() && is_code(b)) {
                    remember_export(cache, it->second, reinterpret_cast<void*>(b));
                } else if (const auto it2 = strings.find(b); it2 != strings.end() && is_code(a)) {
                    remember_export(cache, it2->second, reinterpret_cast<void*>(a));
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
}

inline void collect_lea_name_slots(const ModuleView& view,
                                   std::unordered_map<std::string, void*>& cache) {
    if (!view.exec_start) {
        return;
    }

    auto* begin = reinterpret_cast<std::uint8_t*>(view.exec_start);
    auto* end = reinterpret_cast<std::uint8_t*>(view.exec_end);
    if (end <= begin + 16) {
        return;
    }

    for (auto* p = begin; p + 7 < end; ++p) {
        // lea r64, [rip+disp32]  — 48 8D 05/0D/15/1D/25/2D/35/3D
        if (p[0] != 0x48 || p[1] != 0x8D) {
            continue;
        }
        const std::uint8_t modrm = p[2];
        if ((modrm & 0xC7) != 0x05) {
            continue;
        }
        const auto disp = *reinterpret_cast<const std::int32_t*>(p + 3);
        const auto name_addr = reinterpret_cast<std::uintptr_t>(p + 7 + disp);
        if (!in_module(view, name_addr)) {
            continue;
        }
        const char* name = reinterpret_cast<const char*>(name_addr);
        if (!is_il2cpp_export_name(name)) {
            continue;
        }

        // Look nearby for mov [rip+disp], r64 — the slot already holds the API after init.
        auto* window_end = p + 0x60;
        if (window_end > end) {
            window_end = end;
        }
        for (auto* q = p; q + 7 < window_end; ++q) {
            if (q[0] != 0x48 || q[1] != 0x89) {
                continue;
            }
            if ((q[2] & 0xC7) != 0x05) {
                continue;
            }
            const auto slot_disp = *reinterpret_cast<const std::int32_t*>(q + 3);
            const auto slot = reinterpret_cast<std::uintptr_t>(q + 7 + slot_disp);
            if (!in_module(view, slot) || !safe_readable(reinterpret_cast<void*>(slot), 8)) {
                continue;
            }
            const auto fn = *reinterpret_cast<void**>(slot);
            if (!fn) {
                continue;
            }
            remember_export(cache, name, fn);
            break;
        }
    }
}

inline const char* const* candidate_modules() {
    static const char* kModules[] = {
        config::kGameAssemblyModule,
        config::kUnityPlayerModule,
        "UserAssembly.dll",
        nullptr
    };
    return kModules;
}

inline std::unordered_map<std::string, void*>& export_cache() {
    static std::unordered_map<std::string, void*> cache;
    return cache;
}

inline bool& export_cache_ready() {
    static bool ready = false;
    return ready;
}

inline void build_export_cache() {
    if (export_cache_ready()) {
        return;
    }
    export_cache_ready() = true;

    auto& cache = export_cache();
    cache.reserve(256);

    std::vector<ModuleView> views;
    std::unordered_map<std::uintptr_t, const char*> strings;

    for (const char* const* it = candidate_modules(); *it; ++it) {
        const HMODULE module = GetModuleHandleA(*it);
        if (!module) {
            continue;
        }
        ModuleView view{};
        view.name = *it;
        if (!fill_module_view(module, view)) {
            continue;
        }
        LOG_INFO("IL2CPP", "Scanning %s @ %p", *it, reinterpret_cast<void*>(view.base));
        collect_pe_exports(module, cache);
        collect_il2cpp_strings(view, strings);
        views.push_back(view);
    }

    LOG_INFO("IL2CPP", "Found %zu il2cpp_* name strings", strings.size());

    for (const auto& view : views) {
        collect_pointer_tables(view, views, strings, cache);
        // GameAssembly .text is huge; LEA walk is only useful on UnityPlayer-sized modules.
        if (view.name && std::strcmp(view.name, config::kGameAssemblyModule) != 0) {
            collect_lea_name_slots(view, cache);
        }
    }

    LOG_INFO("IL2CPP", "Auto-scan collected %zu il2cpp_* exports", cache.size());
}

inline void* find_named_export(const char* module_name, const char* export_name) {
    const HMODULE module = GetModuleHandleA(module_name);
    if (!module) {
        return nullptr;
    }
    return reinterpret_cast<void*>(GetProcAddress(module, export_name));
}

inline void* find_export_by_name(const std::string& name) {
    for (const char* const* it = candidate_modules(); *it; ++it) {
        if (void* addr = find_named_export(*it, name.c_str())) {
            return addr;
        }
    }

    build_export_cache();
    const auto& cache = export_cache();
    const auto found = cache.find(name);
    return found != cache.end() ? found->second : nullptr;
}

inline void* find_jmp_gadget() {
    // The stub preserves rdi as &shell_params. Only jmp [rdi] is valid.
    for (const char* const* it = candidate_modules(); *it; ++it) {
        if (void* found = scan_module(*it, "\xFF\x27", "xx")) {
            LOG_INFO("IL2CPP", "Using jmp [rdi] gadget %p in %s", found, *it);
            return found;
        }
    }

    LOG_WARN("IL2CPP", "No jmp [rdi] gadget - API calls will be direct");
    return nullptr;
}

} // namespace il2cpp_base::il2cpp::detail
