#pragma once

#include <cstdint>

namespace il2cpp_base::il2cpp {

inline constexpr std::uint32_t kFieldAccessMask   = 0x0007;
inline constexpr std::uint32_t kFieldStatic       = 0x0010;
inline constexpr std::uint32_t kFieldReadOnly     = 0x0020;
inline constexpr std::uint32_t kFieldConst        = 0x0040;

inline constexpr std::uint32_t kMethodAccessMask   = 0x0007;
inline constexpr std::uint32_t kMethodStatic       = 0x0010;
inline constexpr std::uint32_t kMethodFinal        = 0x0020;
inline constexpr std::uint32_t kMethodVirtual      = 0x0040;
inline constexpr std::uint32_t kMethodNewSlot      = 0x0100;
inline constexpr std::uint32_t kMethodAbstract     = 0x0400;
inline constexpr std::uint32_t kMethodPInvokeImpl  = 0x2000;

} // namespace il2cpp_base::il2cpp
