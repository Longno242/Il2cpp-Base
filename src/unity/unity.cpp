#include "unity/unity.hpp"

#include "core/logger.hpp"
#include "il2cpp/helpers.hpp"

namespace il2cpp_base::unity {
namespace {

using il2cpp::Method;

struct Api {
    il2cpp::Class* object = nullptr;
    Method* find_by_type = nullptr;
    Method* find_of_type = nullptr;
    Method* get_name = nullptr;
    Method* get_transform = nullptr;
    Method* get_game_object = nullptr;
    Method* get_position = nullptr;
    Method* set_position = nullptr;
    Method* get_main_camera = nullptr;
    Method* get_fov = nullptr;
    Method* set_fov = nullptr;
    Method* world_to_screen = nullptr;
    Method* screen_width = nullptr;
    Method* screen_height = nullptr;
    Method* product_name = nullptr;
    Method* unity_version = nullptr;
    Method* get_target_fps = nullptr;
    Method* set_target_fps = nullptr;
    Method* get_vsync = nullptr;
    Method* set_vsync = nullptr;
    Method* get_time_scale = nullptr;
    Method* set_time_scale = nullptr;
    Method* get_gravity = nullptr;
    Method* set_gravity = nullptr;
    Method* get_fog = nullptr;
    Method* set_fog = nullptr;
};

Api api;

il2cpp::Class* engine_class(const char* name) {
    return il2cpp::search_class(name, "UnityEngine");
}

Method* method(il2cpp::Class* klass, const char* name, int args) {
    Method* m = klass ? klass->method(name, args) : nullptr;
    return m && m->address() ? m : nullptr;
}

template <typename T>
T get(Method* m, void* self = nullptr) {
    return m ? il2cpp::unbox<T>(m->invoke(self)) : T{};
}

void* get_object(Method* m, void* self = nullptr) {
    return m ? m->invoke(self) : nullptr;
}

template <typename T>
void set(Method* m, void* self, T value) {
    if (m) {
        void* args[] = { &value };
        m->invoke(self, args);
    }
}

} // namespace

void init() {
    il2cpp::Class* object = engine_class("Object");
    il2cpp::Class* component = engine_class("Component");
    il2cpp::Class* transform = engine_class("Transform");
    il2cpp::Class* camera = engine_class("Camera");
    il2cpp::Class* screen = engine_class("Screen");
    il2cpp::Class* application = engine_class("Application");
    il2cpp::Class* quality = engine_class("QualitySettings");
    il2cpp::Class* time = engine_class("Time");
    il2cpp::Class* physics = engine_class("Physics");
    il2cpp::Class* render_settings = engine_class("RenderSettings");

    api.object = object;
    api.find_by_type = il2cpp::method_with_params(object, "FindObjectsByType",
        { "System.Type", "UnityEngine.FindObjectsSortMode" });
    api.find_of_type = il2cpp::method_with_params(object, "FindObjectsOfType", { "System.Type" });
    api.get_name = method(object, "get_name", 0);
    api.get_transform = method(component, "get_transform", 0);
    api.get_game_object = method(component, "get_gameObject", 0);
    api.get_position = method(transform, "get_position", 0);
    api.set_position = method(transform, "set_position", 1);
    api.get_main_camera = method(camera, "get_main", 0);
    api.get_fov = method(camera, "get_fieldOfView", 0);
    api.set_fov = method(camera, "set_fieldOfView", 1);
    api.world_to_screen = il2cpp::method_with_params(camera, "WorldToScreenPoint", { "UnityEngine.Vector3" });
    api.screen_width = method(screen, "get_width", 0);
    api.screen_height = method(screen, "get_height", 0);
    api.product_name = method(application, "get_productName", 0);
    api.unity_version = method(application, "get_unityVersion", 0);
    api.get_target_fps = method(application, "get_targetFrameRate", 0);
    api.set_target_fps = method(application, "set_targetFrameRate", 1);
    api.get_vsync = method(quality, "get_vSyncCount", 0);
    api.set_vsync = method(quality, "set_vSyncCount", 1);
    api.get_time_scale = method(time, "get_timeScale", 0);
    api.set_time_scale = method(time, "set_timeScale", 1);
    api.get_gravity = method(physics, "get_gravity", 0);
    api.set_gravity = method(physics, "set_gravity", 1);
    api.get_fog = method(render_settings, "get_fog", 0);
    api.set_fog = method(render_settings, "set_fog", 1);

    const auto* slots = reinterpret_cast<Method* const*>(&api.find_by_type);
    const std::size_t count = (sizeof(Api) - sizeof(api.object)) / sizeof(Method*);
    std::size_t found = 0;
    for (std::size_t i = 0; i < count; ++i) {
        found += slots[i] != nullptr;
    }
    LOG_INFO("Unity", "resolved %zu/%zu engine methods", found, count);
}

std::vector<void*> find_objects(il2cpp::Class* klass) {
    void* type = il2cpp::type_object(klass);
    if (!type) {
        return {};
    }
    void* array = nullptr;
    if (api.find_by_type) {
        int sort_none = 0;
        void* args[] = { type, &sort_none };
        array = api.find_by_type->invoke(nullptr, args);
    } else if (api.find_of_type) {
        void* args[] = { type };
        array = api.find_of_type->invoke(nullptr, args);
    }
    return il2cpp::array_to_vector<void*>(array);
}

std::string name(void* object) {
    return object ? il2cpp::string_from_il2cpp(get_object(api.get_name, object)) : std::string{};
}

void* transform(void* component) {
    return component ? get_object(api.get_transform, component) : nullptr;
}

void* game_object(void* component) {
    return component ? get_object(api.get_game_object, component) : nullptr;
}

Vector3 position(void* transform) {
    return transform ? get<Vector3>(api.get_position, transform) : Vector3{};
}

void set_position(void* transform, Vector3 position) {
    if (transform) {
        set(api.set_position, transform, position);
    }
}

void* main_camera() {
    return get_object(api.get_main_camera);
}

float field_of_view(void* camera) {
    return camera ? get<float>(api.get_fov, camera) : 0.0f;
}

void set_field_of_view(void* camera, float fov) {
    if (camera) {
        set(api.set_fov, camera, fov);
    }
}

bool world_to_screen(void* camera, Vector3 world, Vector3& screen) {
    if (!camera || !api.world_to_screen) {
        return false;
    }
    void* args[] = { &world };
    void* boxed = api.world_to_screen->invoke(camera, args);
    if (!boxed) {
        return false;
    }
    screen = il2cpp::unbox<Vector3>(boxed);
    return screen.z > 0.0f;
}

int screen_width() { return get<int>(api.screen_width); }
int screen_height() { return get<int>(api.screen_height); }
std::string product_name() { return il2cpp::string_from_il2cpp(get_object(api.product_name)); }
std::string unity_version() { return il2cpp::string_from_il2cpp(get_object(api.unity_version)); }
int target_frame_rate() { return get<int>(api.get_target_fps); }
void set_target_frame_rate(int fps) { set(api.set_target_fps, nullptr, fps); }
int vsync_count() { return get<int>(api.get_vsync); }
void set_vsync_count(int count) { set(api.set_vsync, nullptr, count); }
float time_scale() { return api.get_time_scale ? get<float>(api.get_time_scale) : 1.0f; }
void set_time_scale(float scale) { set(api.set_time_scale, nullptr, scale); }
Vector3 gravity() { return get<Vector3>(api.get_gravity); }
void set_gravity(Vector3 gravity) { set(api.set_gravity, nullptr, gravity); }
bool fog() { return get<bool>(api.get_fog); }
void set_fog(bool enabled) { set(api.set_fog, nullptr, enabled); }

} // namespace il2cpp_base::unity
