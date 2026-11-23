#include "features/field_override.hpp"

#include <cstring>

#include "il2cpp/helpers.hpp"

namespace il2cpp_base::features {

FieldOverride::FieldOverride(const char* default_field, float default_value) : value(default_value) {
    strncpy_s(field, default_field, _TRUNCATE);
}

void FieldOverride::apply(void* object, il2cpp::Class* klass) {
    if (!enabled) {
        status = "off";
        return;
    }
    if (!object || !klass) {
        status = "no local player";
        return;
    }

    if (klass != resolved_class_ || std::strcmp(field, resolved_name_) != 0) {
        strncpy_s(resolved_name_, field, _TRUNCATE);
        resolved_class_ = klass;
        field_ = klass->field(resolved_name_);
    }
    if (!field_ || field_->is_static()) {
        status = "field not found";
        return;
    }

    const std::string& type = field_->type();
    if (type == "System.Single") {
        field_->instance_value<float>(object) = value;
    } else if (type == "System.Int32") {
        field_->instance_value<int>(object) = static_cast<int>(value);
    } else if (type == "System.Double") {
        field_->instance_value<double>(object) = value;
    } else if (type == "System.Boolean") {
        field_->instance_value<bool>(object) = value != 0.0f;
    } else {
        status = "unsupported field type";
        return;
    }
    status = "active";
}

} // namespace il2cpp_base::features
