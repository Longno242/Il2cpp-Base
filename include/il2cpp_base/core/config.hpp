#pragma once

#include <cstdint>
#include <string>

namespace il2cpp_base::config {

inline constexpr const char* kGameAssemblyModule = "GameAssembly.dll";
inline constexpr const char* kUnityPlayerModule   = "UnityPlayer.dll";

inline constexpr const char* kLogFileName = "Il2cppBase.log";
inline constexpr bool        kMirrorLogToConsole = true;

inline constexpr int kMenuToggleKey = 0x2D; // INSERT
inline constexpr const char* kMenuTitle  = "Il2cppBase";
inline constexpr const char* kMenuHeader = "IL2CPP BASE";

} // namespace il2cpp_base::config

// fill these in for the example MinHook sample (or leave empty)
namespace il2cpp_base::game {

inline constexpr const char* kExampleAssembly = "";
inline constexpr const char* kExampleNamespace = "";
inline constexpr const char* kExampleClass = "";
inline constexpr const char* kExampleMethod = "";
inline constexpr int kExampleMethodArgCount = 0;

} // namespace il2cpp_base::game
