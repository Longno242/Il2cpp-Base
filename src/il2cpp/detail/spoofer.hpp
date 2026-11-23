#pragma once

// Return-address stub (adapted from rrid / ret_addr_spoofer)
// https://github.com/lennyRBLX/ret_addr_spoofer
//
// Needs a jmp [rdi] gadget (FF 27). The stub leaves rdi = shell_params, and
// rdi is non-volatile in the Microsoft x64 ABI. jmp [rdx] is not safe here.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <Windows.h>

#include "core/logger.hpp"

namespace il2cpp_base::il2cpp::detail::spoofer {

inline std::atomic<int>& mode() {
    // 0 = try spoof, 1 = spoof works, -1 = fall back to direct calls
    static std::atomic<int> value{ 0 };
    return value;
}

inline void disable(const char* reason) {
    const int previous = mode().exchange(-1);
    if (previous >= 0) {
        LOG_WARN("IL2CPP", "Spoofer disabled (%s) - using direct API calls", reason);
    }
}

namespace internal {

#pragma section(".text")
__declspec(allocate(".text")) __declspec(selectany) __declspec(align(16))
inline std::uint8_t stub_asm[] = {
    0x41, 0x5B,
    0x48, 0x83, 0xC4, 0x08,
    0x48, 0x8B, 0x44, 0x24, 0x18,
    0x4C, 0x8B, 0x10,
    0x4C, 0x89, 0x14, 0x24,
    0x4C, 0x8B, 0x50, 0x08,
    0x4C, 0x89, 0x58, 0x08,
    0x48, 0x89, 0x78, 0x10,
    0x48, 0x8D, 0x3D, 0x09, 0x00, 0x00, 0x00,
    0x48, 0x89, 0x38,
    0x48, 0x8B, 0xF8,
    0x41, 0xFF, 0xE2,
    0x48, 0x83, 0xEC, 0x10,
    0x48, 0x8B, 0xCF,
    0x48, 0x8B, 0x79, 0x10,
    0xFF, 0x61, 0x08,
};

inline auto stub_fn() {
    return reinterpret_cast<void(*)()>(&stub_asm);
}

template<typename Ret, typename... Args>
static inline auto call_helper(const void* shell, Args... args) -> Ret {
    return reinterpret_cast<Ret(*)(Args...)>(shell)(args...);
}

template<std::size_t Argc, typename = void>
struct argument_remapper;

template<std::size_t Argc>
struct argument_remapper<Argc, std::enable_if_t<Argc <= 4>> {
    template<typename Ret, typename First = void*, typename Second = void*,
             typename Third = void*, typename Fourth = void*>
    static auto do_call(const void* shell, void* shell_param,
                        First first = First{}, Second second = Second{},
                        Third third = Third{}, Fourth fourth = Fourth{}) -> Ret {
        return call_helper<Ret, First, Second, Third, Fourth, void*, void*>(
            shell, first, second, third, fourth, shell_param, nullptr);
    }
};

template<std::size_t Argc>
struct argument_remapper<Argc, std::enable_if_t<(Argc > 4)>> {
    template<typename Ret, typename First, typename Second, typename Third,
             typename Fourth, typename... Pack>
    static auto do_call(const void* shell, void* shell_param,
                        First first, Second second, Third third, Fourth fourth,
                        Pack... pack) -> Ret {
        return call_helper<Ret, First, Second, Third, Fourth, void*, void*, Pack...>(
            shell, first, second, third, fourth, shell_param, nullptr, pack...);
    }
};

template<typename Result, typename... Arguments>
inline Result spoof(const void* trampoline, Result(*fn)(Arguments...),
                    std::type_identity_t<Arguments>... args) {
    struct shell_params {
        const void* trampoline;
        void* function;
        void* register_;
    };

    shell_params params{ trampoline, reinterpret_cast<void*>(fn) };
    using mapper = argument_remapper<sizeof...(Arguments)>;
    return mapper::template do_call<Result, Arguments...>(
        stub_fn(), &params, args...);
}

} // namespace internal

template<typename Result, typename... Arguments>
inline Result call(const void* trampoline, Result(*fn)(Arguments...),
                   std::type_identity_t<Arguments>... args) {
    if (!fn) {
        if constexpr (std::is_void_v<Result>) {
            return;
        } else {
            return Result{};
        }
    }

    if (trampoline && mode().load() >= 0) {
        __try {
            if constexpr (std::is_void_v<Result>) {
                internal::spoof(trampoline, fn, args...);
                mode().store(1);
                return;
            } else {
                Result result = internal::spoof(trampoline, fn, args...);
                mode().store(1);
                return result;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            disable("spoofed call raised");
        }
    }

    return fn(args...);
}

} // namespace il2cpp_base::il2cpp::detail::spoofer
