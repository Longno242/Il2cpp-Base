#pragma once

#include <cmath>
#include <string>
#include <vector>

#include "il2cpp/types.hpp"

// Thin wrappers over common UnityEngine APIs. Main thread only, like all game calls.
// Everything returns a safe default if the game strips the method.
namespace il2cpp_base::unity {

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

inline bool operator==(Vector3 a, Vector3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
inline Vector3 operator+(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vector3 operator-(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline float length(Vector3 v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
inline float distance(Vector3 a, Vector3 b) { return length(a - b); }

void init();

// Objects
std::vector<void*> find_objects(il2cpp::Class* klass);
std::string name(void* object);
void* transform(void* component);
void* game_object(void* component);

// Transform
Vector3 position(void* transform);
void set_position(void* transform, Vector3 position);

// Camera
void* main_camera();
float field_of_view(void* camera);
void set_field_of_view(void* camera, float fov);
// Pixel position with a bottom-left origin; z is the depth in front of the camera.
bool world_to_screen(void* camera, Vector3 world, Vector3& screen);

// Screen, application, quality
int screen_width();
int screen_height();
std::string product_name();
std::string unity_version();
int target_frame_rate();
void set_target_frame_rate(int fps);
int vsync_count();
void set_vsync_count(int count);

// Time and physics
float time_scale();
void set_time_scale(float scale);
Vector3 gravity();
void set_gravity(Vector3 gravity);

// Rendering
bool fog();
void set_fog(bool enabled);

} // namespace il2cpp_base::unity
