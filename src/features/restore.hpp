#pragma once

namespace il2cpp_base::features {

// Forces a game value while enabled and puts the game's own value back when disabled.
template <typename T>
struct Restorable {
    bool active = false;
    T original{};

    template <typename Get, typename Set>
    void apply(bool enabled, const T& value, Get&& get, Set&& set) {
        if (enabled) {
            if (!active) {
                original = get();
                active = true;
            }
            if (!(get() == value)) {
                set(value);
            }
        } else if (active) {
            active = false;
            set(original);
        }
    }
};

} // namespace il2cpp_base::features
