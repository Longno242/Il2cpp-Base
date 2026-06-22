#pragma once

// Return-address spoofer (adapted from rrid / ret_addr_spoofer)
// https://github.com/lennyRBLX/ret_addr_spoofer

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace il2cpp_base::il2cpp::detail::spoofer {

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

} // namespace internal

template<typename Result, typename... Arguments>
inline Result call(const void* trampoline, Result(*fn)(Arguments...),
                   std::type_identity_t<Arguments>... args) {
    struct shell_params {
        const void* trampoline;
        void* function;
        void* register_;
    };

    shell_params params{ trampoline, reinterpret_cast<void*>(fn) };
    using mapper = internal::argument_remapper<sizeof...(Arguments)>;
    return mapper::template do_call<Result, Arguments...>(
        internal::stub_fn(), &params, args...);
}

} // namespace il2cpp_base::il2cpp::detail::spoofer
