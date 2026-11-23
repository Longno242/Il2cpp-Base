#pragma once

#include <string>
#include <vector>

#include "config.hpp"
#include "core/logger.hpp"
#include "il2cpp/detail/api.hpp"
#include "il2cpp/detail/scan.hpp"
#include "il2cpp/detail/spoofer.hpp"
#include "il2cpp/types.hpp"
#include "il2cpp/types_impl.hpp"

namespace il2cpp_base::il2cpp {

[[nodiscard]] inline bool init() {
    auto& c = detail::ctx();
    if (c.initialized) {
        return true;
    }

    if (!detail::resolve_api()) {
        return false;
    }

    c.jmp_rdx = detail::find_jmp_gadget();

    void* domain = detail::spoofer::call(c.jmp_rdx, c.api.domain_get);
    if (!domain) {
        LOG_FATAL("IL2CPP", "il2cpp_domain_get returned null");
        return false;
    }
    detail::spoofer::call(c.jmp_rdx, c.api.thread_attach, domain);

    std::size_t assembly_count = 0;
    const void** assemblies = detail::spoofer::call(
        c.jmp_rdx, c.api.get_assemblies, static_cast<const void*>(domain), &assembly_count);
    if (!assemblies) {
        LOG_FATAL("IL2CPP", "il2cpp_domain_get_assemblies returned null");
        return false;
    }

    c.images.reserve(assembly_count);
    c.image_lookup.reserve(assembly_count);

    for (std::size_t i = 0; i < assembly_count; ++i) {
        const void* assembly = assemblies[i];
        if (!assembly) {
            continue;
        }

        const void* image = detail::spoofer::call(c.jmp_rdx, c.api.get_image, assembly);
        if (!image) {
            continue;
        }

        const char* name = detail::spoofer::call(c.jmp_rdx, c.api.get_image_name, image);
        if (!name || !*name) {
            continue;
        }

        auto img = std::make_unique<Image>(name, image);
        c.image_lookup[name] = img.get();
        c.images.push_back(std::move(img));
    }

    c.initialized = true;
    LOG_INFO("IL2CPP", "Resolver ready - %zu assemblies loaded", c.images.size());
    return true;
}

[[nodiscard]] inline bool ready() {
    return detail::ctx().initialized;
}

inline void ensure_thread() {
    auto& c = detail::ctx();
    if (!c.initialized || !c.api.thread_attach || !c.api.domain_get) {
        return;
    }
    if (void* domain = detail::spoofer::call(c.jmp_rdx, c.api.domain_get)) {
        detail::spoofer::call(c.jmp_rdx, c.api.thread_attach, domain);
    }
}

[[nodiscard]] inline Image* image(const std::string& name) {
    auto& c = detail::ctx();
    if (!c.initialized) {
        return nullptr;
    }
    const auto it = c.image_lookup.find(name);
    return it != c.image_lookup.end() ? it->second : nullptr;
}

[[nodiscard]] inline const std::vector<Image*>& images() {
    static std::vector<Image*> result;
    auto& c = detail::ctx();

    if (!c.initialized) {
        result.clear();
        return result;
    }

    if (result.size() != c.images.size()) {
        result.clear();
        result.reserve(c.images.size());
        for (auto& img : c.images) {
            result.push_back(img.get());
        }
    }

    return result;
}

// Convenience: resolve class from assembly image name.
[[nodiscard]] inline Class* find_class(const std::string& image_name,
                                       const std::string& class_name,
                                       const std::string& namespaze = "") {
    Image* img = image(image_name);
    return img ? img->klass(class_name, namespaze) : nullptr;
}

} // namespace il2cpp_base::il2cpp
