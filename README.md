# Il2cpp Base

C++ starter project for IL2CPP Unity mods. Built for single-player games.

Comes with a runtime class/method resolver (based on my old [rrid](https://github.com/Longno242/IL2CPP-Dumper/blob/main/IL2CPP%20Dumper/rrid.hpp) work), MinHook, ImGui overlay, and file logging.

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

That script pulls MinHook and Dear ImGui into `vendor/`.

## Build

Open the folder in VS Code and run the **CMake: Build Release** task, or from a terminal:

```powershell
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

DLL output: `build/bin/Release/Il2cppBase.dll`

If you're on VS 2022, swap the generator to `"Visual Studio 17 2022"`.

## Inject

Load the DLL after `GameAssembly.dll` is in memory. Use whatever injector you already use.

Logs go to `Il2cppBase.log` next to the game exe. A console window also opens by default.

Menu toggle is **INSERT** (change in `include/il2cpp_base/core/config.hpp`).

## Resolver example

```cpp
#include "il2cpp_base/il2cpp/resolver.hpp"

il2cpp::init();

auto* player = il2cpp::find_class("Assembly-CSharp.dll", "PlayerController", "Game");
auto* health = player->field("health");

float hp = health->instance_value<float>(player_obj);

auto* take_damage = player->method("TakeDamage", 1);
take_damage->call<void>(player_obj, 25.0f);
```

## Hook example

```cpp
#include "il2cpp_base/core/hooks.hpp"

using update_t = void(__fastcall*)(void*);
update_t original_update = nullptr;

void __fastcall hk_update(void* self) {
    // stuff here
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
  core/       logger, hooks, config
  il2cpp/     resolver headers
  ui/         imgui overlay
src/
  dllmain.cpp
  ui/overlay.cpp
```

## Patterns

The resolver finds IL2CPP APIs by scanning `GameAssembly.dll` and walking exports in `UnityPlayer.dll`. When Unity updates, patterns can break.

If init fails, open the log — it'll tell you which export or signature failed. Fix them in `include/il2cpp_base/il2cpp/resolver/detail/api.hpp`.

## Disclaimer

For offline / single-player use. Don't use this on games you don't own or on live services.
