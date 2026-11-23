# Il2cpp Base

C++ base for IL2CPP Unity mods. Single-player games only.

You get a runtime IL2CPP resolver (no dumper needed), MinHook, a DX11 ImGui menu, a class explorer, a crash handler and logging.

Mono games won't work. If the game has no `GameAssembly.dll`, use [Mono-Base](https://github.com/Longno242/Mono-Base).

## Build

Needs Windows x64, Visual Studio 2022 or 2026 with the C++ workload, CMake 3.21+ and Git.

```powershell
git clone https://github.com/Longno242/Il2cpp-Base.git
cd Il2cpp-Base
.\build.ps1
```

The first run downloads Dear ImGui v1.92.9b and MinHook v1.3.4 into `vendor/`. After that it just builds.

- `.\build.ps1 -Debug` for a debug build
- `.\build.ps1 -Clean` to wipe `build/` first

Output: `build/bin/Release/Il2cppBase.dll`

## Use

Inject the DLL with whatever injector you use. It waits for `GameAssembly.dll` on its own.

- **Insert** opens the menu (`kMenuKey` in `src/config.hpp`)
- The log goes to `Il2cppBase.log` next to the game exe, and to a console if `kShowConsole` is on
- While the menu is open the game's cursor is unlocked and clicks over the menu don't reach the game. Both can be turned off in Settings.

## What's in the menu

| Tab | |
| --- | --- |
| Home | game info, quick toggles, getting started |
| Player | local player info, lock health / stamina / ammo / money |
| Movement | speed hack, custom gravity, move speed and jump force |
| Player List | every object of your player class, distance, teleport |
| Visuals | ESP (names, distance, snaplines), FOV, crosshair |
| World | no fog, FPS unlock, VSync |
| Explorer | browse assemblies, classes, fields and methods |
| Log | live log |
| Settings | UI scale, cursor and input options |

Speed, gravity, FOV, fog and FPS use plain UnityEngine APIs and work in most games straight away.

Everything player-related needs your game's player class. Find it in Explorer, type it into Player List and hit Apply (or set `kPlayerClass` in `src/config.hpp`). The closest one to the camera counts as you.

The Player and Movement locks write a field on that class every frame. The field names (`health`, `moveSpeed`, ...) are guesses and can be edited in the menu. Each one shows whether it found the field.

## Adding to the menu

Each tab is its own file in `src/tabs/`, and `src/menu.cpp` lists them in sidebar order. A tab is a function, a section is a card:

```cpp
// src/tabs/combat.cpp
void tabs::combat() {
    ui::section("Aimbot", [] {
        ui::toggle("Enabled", &aim::enabled);
        ui::slider("FOV", &aim::fov, 1.0f, 180.0f, "%.0f");
        ui::combo("Bone", &aim::bone, { "Head", "Neck", "Chest" });
        if (ui::button("Reset")) {
            aim::fov = 90.0f;
            ui::notify("FOV reset");
        }
    });
}

// src/menu.cpp (also declare combat() in src/tabs/tabs.hpp)
constexpr ui::Tab kTabs[] = {
    { "Home", tabs::home },
    { "Combat", tabs::combat },
    // ...
};
```

Sections fill two columns automatically. Pass `1` as the third `Tab` field for a single full-width column.

Widgets (`src/ui/widgets.hpp`):

| Widget | |
| --- | --- |
| `toggle(label, &bool)` | on/off switch |
| `slider(label, &float/&int, min, max, fmt)` | slider |
| `button(label, width)` | full width unless you give one |
| `combo(label, &int, { ... })` | dropdown |
| `segmented(id, &int, { ... })` | inline tab picker |
| `color(label, float[4])` | color picker |
| `input(hint, &std::string)` | text box |
| `text(fmt, ...)` / `value(label, fmt, ...)` | dim text / label + value row |
| `notify(fmt, ...)` | toast in the corner, safe from any thread |

Plain ImGui calls work anywhere inside a section too. Colors live in `src/ui/style.hpp`.

## Adding a feature

1. Make `src/features/my_feature.hpp/.cpp` (any `.cpp` under `src/` is picked up by CMake)
2. Call its `install()` and `update()` from `src/features/features.cpp`
3. Draw its settings from a tab in `src/tabs/`

`src/features/world.cpp` is a short complete example. `src/unity/unity.hpp` wraps the common UnityEngine calls (camera, transforms, `FindObjectsOfType`, screen, time, physics).

### Threads

The menu draws on the render thread. Calling into the game from there will crash sooner or later. `install()` and `update()` already run on Unity's main thread. From a button, queue the work:

```cpp
if (ui::button("Give money")) {
    main_thread::post([] {
        money_field->set_static<int>(999999);
    });
}
```

### Resolver

```cpp
il2cpp::Image* game = il2cpp::image("Assembly-CSharp.dll");
il2cpp::Class* player = game->klass("PlayerController", "Game");

float hp = player->field("health")->instance_value<float>(player_obj);
player->method("TakeDamage", 1)->invoke(player_obj, args);
```

Handy helpers in `src/il2cpp/helpers.hpp`: `search_class` (every assembly), `method_with_params` (pick an overload), `get_property` / `set_property`, `type_object`, `array_to_vector`, `gc_hold` / `gc_release`, `resolve_icall`, `object_to_string`.

Use the Explorer tab in the menu to browse assemblies, classes, fields (with offsets) and methods (with RVAs). Click a row to copy the name.

### Hooks

```cpp
using update_fn = void (*)(void* self, const void* method);
update_fn original = nullptr;

void hk_update(void* self, const void* method) {
    original(self, method);
}

hooks::install("Player.Update", player->method("Update", 0)->address(), &hk_update, &original);
hooks::set_enabled("Player.Update", false);
```

IL2CPP methods take `this` first (instance methods only) and a `MethodInfo*` last. `src/features/example_hook.cpp` has a full one with an on/off toggle in the menu.

## Layout

```
src/
  main.cpp        startup and shutdown
  config.hpp      name, menu key, log file, player class
  menu.cpp        sidebar tab list
  tabs/           one file per tab
  features/       the actual cheats, run on the main thread
  unity/          UnityEngine wrappers
  core/           logger, hooks, main thread queue, crash handler
  il2cpp/         resolver
  ui/             overlay, widgets, style, explorer and log pages
```

## How the resolver finds IL2CPP

Tried in order until it has everything:

1. `GetProcAddress` on `GameAssembly.dll`, `UnityPlayer.dll`, `UserAssembly.dll`
2. Walking their export tables
3. Scanning for `il2cpp_*` name strings and the `{name, function}` tables next to them
4. LEA/MOV slots in UnityPlayer
5. Byte patterns for `domain_get` / `thread_attach`

If init fails the log names every export it couldn't find.

## Disclaimer

For offline and single-player games you own. Don't use it on live services.
