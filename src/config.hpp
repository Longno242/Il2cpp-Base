#pragma once

namespace il2cpp_base::config {

inline constexpr const char* kName = "Il2cpp Base";

// Virtual-key code that opens and closes the menu.
inline constexpr int kMenuKey = 0x2D; // VK_INSERT

inline constexpr const char* kLogFileName = "Il2cppBase.log";
inline constexpr bool kShowConsole = true;

// Class the Player List looks for. "Namespace.Class" or just "Class". Can also be changed in the menu.
inline constexpr const char* kPlayerClass = "PlayerController";

inline constexpr const char* kGameAssemblyModule = "GameAssembly.dll";
inline constexpr const char* kUnityPlayerModule = "UnityPlayer.dll";

} // namespace il2cpp_base::config
