#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "il2cpp_base/il2cpp/resolver/flags.hpp"

namespace il2cpp_base::il2cpp {

class Field;
class Method;

class Class {
public:
    Class(std::string name, std::string namespaze, const void* klass);

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::string& namespaze() const { return namespaze_; }
    [[nodiscard]] const void* raw() const { return klass_; }
    [[nodiscard]] std::uintptr_t rva() const;
    [[nodiscard]] std::uintptr_t module_base() const { return module_base_; }

    [[nodiscard]] bool is_enum();
    [[nodiscard]] bool is_generic();
    [[nodiscard]] bool is_value_type();

    [[nodiscard]] Field* field(const std::string& name);
    [[nodiscard]] const std::vector<Field*>& fields();

    [[nodiscard]] Method* method(const std::string& name, int arg_count = -1);
    [[nodiscard]] const std::vector<Method*>& methods();

    [[nodiscard]] Class* nested(const std::string& name);
    [[nodiscard]] const std::vector<Class*>& nested_types();

private:
    [[nodiscard]] std::string type_name(const void* type);

    std::string name_;
    std::string namespaze_;
    const void* klass_ = nullptr;
    std::uintptr_t module_base_ = 0;

    std::vector<std::unique_ptr<Field>> owned_fields_;
    std::vector<Field*> field_ptrs_;
    std::unordered_map<std::uint64_t, Field*> field_cache_;

    std::vector<std::unique_ptr<Method>> owned_methods_;
    std::vector<Method*> method_ptrs_;

    std::vector<std::unique_ptr<Class>> owned_nested_;
    std::vector<Class*> nested_ptrs_;

    bool fields_ready_ = false;
    bool methods_ready_ = false;
    bool nested_ready_ = false;
};

class Image {
public:
    explicit Image(std::string name, const void* image);

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const void* raw() const { return image_; }
    [[nodiscard]] std::uintptr_t rva() const;
    [[nodiscard]] std::uintptr_t module_base() const { return module_base_; }

    [[nodiscard]] Class* klass(const std::string& name, const std::string& namespaze = "");
    [[nodiscard]] const std::vector<Class*>& classes();

private:
    std::string name_;
    const void* image_ = nullptr;
    std::uintptr_t module_base_ = 0;

    std::vector<std::unique_ptr<Class>> owned_classes_;
    std::vector<Class*> class_ptrs_;
    std::unordered_map<std::uint64_t, Class*> class_cache_;
    bool classes_ready_ = false;
};

class Field {
public:
    Field(std::string name, std::string type, std::size_t offset, std::uint32_t flags, const void* field);

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::string& type() const { return type_; }
    [[nodiscard]] std::uint32_t flags() const { return flags_; }
    [[nodiscard]] std::size_t offset() const { return offset_; }
    [[nodiscard]] const void* raw() const { return field_; }
    [[nodiscard]] std::uintptr_t static_rva() const { return static_rva_; }
    [[nodiscard]] bool is_static() const { return (flags_ & kFieldStatic) != 0; }

    template<typename T>
    T get_static() const;

    template<typename T>
    void set_static(const T& value) const;

    template<typename T>
    T& instance_value(void* object) const {
        return *reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(object) + offset_);
    }

    [[nodiscard]] std::string describe() const;

private:
    std::string name_;
    std::string type_;
    std::size_t offset_ = 0;
    std::uint32_t flags_ = 0;
    const void* field_ = nullptr;
    std::uintptr_t static_rva_ = 0;
};

class Method {
public:
    Method(std::string name, std::string return_type,
           std::vector<std::pair<std::string, std::string>> params,
           std::uint32_t flags, void* address, const void* method_info);

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::string& return_type() const { return return_type_; }
    [[nodiscard]] std::uint32_t flags() const { return flags_; }
    [[nodiscard]] void* address() const { return address_; }
    [[nodiscard]] const void* raw() const { return method_info_; }
    [[nodiscard]] std::uintptr_t rva() const { return rva_; }
    [[nodiscard]] std::size_t param_count() const { return params_.size(); }
    [[nodiscard]] const auto& params() const { return params_; }
    [[nodiscard]] bool is_static() const { return (flags_ & kMethodStatic) != 0; }

    template<typename Ret, typename... Args>
    Ret call(Args... args) const {
        using fn_t = Ret(*)(Args...);
        return reinterpret_cast<fn_t>(address_)(args...);
    }

    void* invoke(void* instance, void** params = nullptr) const;

    [[nodiscard]] std::string describe() const;

private:
    std::string name_;
    std::string return_type_;
    std::uint32_t flags_ = 0;
    std::vector<std::pair<std::string, std::string>> params_;
    void* address_ = nullptr;
    const void* method_info_ = nullptr;
    std::uintptr_t rva_ = 0;
};

} // namespace il2cpp_base::il2cpp
