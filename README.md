# Il2cpp Base

C++ starter project for IL2CPP Unity mods. Built for single-player games.

Comes with a runtime class/method resolver (based on my old [rrid](https://github.com/Longno242/IL2CPP-Dumper/blob/main/IL2CPP%20Dumper/rrid.hpp) work), MinHook, ImGui overlay, crash handler, and file logging.

**Mono games won't work** — if the game does not use `GameAssembly.dll`, use [Mono-Base](https://github.com/Longno242/Mono-Base) instead.

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 or newer (C++ desktop workload)
- CMake 3.20+
- Git

## Setup

```powershell
git clone https://github.com/Longno242/Il2cpp-Base.git
cd Il2cpp-Base
.\scripts\setup_deps.ps1
```

That script pulls pinned MinHook **v1.3.4** and Dear ImGui **v1.92.9b** into `vendor/`. Re-run it to refresh an existing clone.

## Build

Open the folder in VS Code and run the **CMake: Build Release** task, or from a terminal:

```powershell
.\scripts\configure.ps1
cmake --build build --config Release
```

`configure.ps1` tries Visual Studio 2026, then 2022. You can also pass a generator yourself:

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

DLL output: `build/bin/Release/Il2cppBase.dll`

## Inject

Load the DLL after `GameAssembly.dll` is in memory. Use whatever injector you already use.

Logs go to `Il2cppBase.log` next to the game exe. A console window also opens by default (`kMirrorLogToConsole`).

Menu toggle is **INSERT** (change in `include/il2cpp_base/core/config.hpp`).

## Resolver example

```cpp
#include "il2cpp_base/il2cpp/resolver.hpp"

il2cpp::init();

auto* player = il2cpp::find_class("Assembly-CSharp.dll", "PlayerController", "Game");
auto* health = player->field("health");

float hp = health->instance_value<float>(player_obj);

auto* take_damage = player->method("TakeDamage", 1);
take_damage->invoke(player_obj, /* params */);
```

Prefer `Method::invoke()` from the game thread. Direct `call<>()` jumps to the native pointer and is only for cases you already understand.

## Hook example

Fill `game::kExample*` in `config.hpp`, or hook from your own feature:

```cpp
#include "il2cpp_base/core/hooks.hpp"

using update_t = void(__fastcall*)(void*);
update_t original_update = nullptr;

void __fastcall hk_update(void* self) {
    original_update(self);
}

void install_hook(il2cpp::Method* update_method) {
    auto& hooks = il2cpp_base::hooks::HookManager::instance();
    hooks.create("Player.Update", update_method->address(), &hk_update, &original_update);
    hooks.enable("Player.Update");
}
```

## Project structure

```
include/il2cpp_base/
  core/       logger, hooks, config, crash handler
  il2cpp/     resolver + helpers
  ui/         imgui overlay + menu
  features/   example hook
src/
  dllmain.cpp
  core/crash_handler.cpp
  ui/overlay.cpp
  ui/menu.cpp
  features/example_hook.cpp
```

## Patterns

The resolver auto-detects `il2cpp_*` APIs, in order:

1. `GetProcAddress` on `GameAssembly.dll`, `UnityPlayer.dll`, and `UserAssembly.dll`
2. PE export-directory walk on those modules
3. Scan for `il2cpp_*` name strings and nearby `{name, function}` pointer tables
4. LEA/MOV slot walk in UnityPlayer
5. Last-resort byte patterns for `domain_get` / `thread_attach`

If init fails, the log lists each missing export by name. `MethodInfo::methodPointer` at offset `0` is treated as valid (that is the usual layout).

## Stability notes

- Don't call IL2CPP / `runtime_invoke` from Present. Queue work and run it in `pump_main_thread_features()` (WndProc). Present posts that message automatically.
- `Method::invoke()` attaches the IL2CPP thread and logs managed exceptions instead of letting them tear the process down.
- Static fields use `il2cpp_field_static_get_value` / `set_value`, not the FieldInfo pointer.
- Closing the log console should not kill the game (console ctrl handler).
- `HookManager::remove()` / `shutdown()` no longer deadlock on the internal mutex.

## Disclaimer

For offline / single-player use. Don't use this on games you don't own or on live services.
