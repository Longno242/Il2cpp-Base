#pragma once

// A worked example of hooking a game method. Point the constants in example_hook.cpp at a real
// instance method with no parameters (an Update() is a good first target) and rebuild.
namespace il2cpp_base::features::example_hook {

inline bool enabled = true;

void install();
void shutdown();
void set_enabled(bool on);

bool installed();
const char* target();
unsigned long long calls();
void* last_instance();

} // namespace il2cpp_base::features::example_hook
