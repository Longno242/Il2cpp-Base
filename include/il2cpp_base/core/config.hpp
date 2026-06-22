#pragma once

#include <cstdint>
#include <string>

namespace il2cpp_base::config {

// Change these per-target game.
inline constexpr const char* kGameAssemblyModule = "GameAssembly.dll";
inline constexpr const char* kUnityPlayerModule   = "UnityPlayer.dll";

inline constexpr const char* kLogFileName = "Il2cppBase.log";
inline constexpr bool        kMirrorLogToConsole = true;

// INSERT -> toggle menu
inline constexpr int kMenuToggleKey = 0x2D;

} // namespace il2cpp_base::config
