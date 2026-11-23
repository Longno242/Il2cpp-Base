#pragma once

#include "il2cpp/types.hpp"

namespace il2cpp_base::features {

// Keeps a numeric field on an object pinned to a value while enabled. The field name can be edited
// from the menu, so the same feature works across games that name things differently.
struct FieldOverride {
    char field[64]{};
    bool enabled = false;
    float value = 0.0f;
    const char* status = "off";

    FieldOverride(const char* default_field, float default_value);

    // Main thread only.
    void apply(void* object, il2cpp::Class* klass);

private:
    char resolved_name_[64]{};
    il2cpp::Class* resolved_class_ = nullptr;
    il2cpp::Field* field_ = nullptr;
};

} // namespace il2cpp_base::features
