#ifndef VE001_LIGHT_H
#define VE001_LIGHT_H

#include <vmath/vmath.h>

#include "camera.h"

namespace ve001 {
    
struct DirectionalLightData {
    alignas(16) vmath::Vec3f32 direction;    
    alignas(16) vmath::Vec3f32 ambient;
    alignas(16) vmath::Vec3f32 diffuse;
    vmath::Vec3f32 specular;
    vmath::f32 shadow_map_index;
    vmath::Mat4f32 transform;
};

struct PointLightData {
    alignas(16) vmath::Vec3f32 position;
    alignas(16) vmath::Vec3f32 ambient;
    alignas(16) vmath::Vec3f32 diffuse;
    alignas(16) vmath::Vec3f32 specular;
    vmath::Vec3f32 attenuation_params;    
    vmath::f32 shadow_map_index;
    vmath::Mat4f32 transform[6];
};

struct SpotLightData {
    alignas(16) vmath::Vec3f32 position;
    alignas(16) vmath::Vec3f32 direction;
    alignas(16) vmath::Vec3f32 ambient;
    alignas(16) vmath::Vec3f32 diffuse;
    alignas(16) vmath::Vec3f32 specular;
    alignas(16) vmath::Vec3f32 attenuation_params;    
    vmath::Vec3f32 cut_off; // inner, outer, unused
    vmath::f32 shadow_map_index;
    vmath::Mat4f32 transform;
};

template<typename T>
struct Light {
    static constexpr vmath::Mat4f32 DEPTH_BIAS_MATRIX {
        vmath::Vec4f32{ .5F, 0.F, 0.F, 0.F},
        vmath::Vec4f32{ 0.F, .5F, 0.F, 0.F},
        vmath::Vec4f32{ 0.F, 0.F, .5F, 0.F},
        vmath::Vec4f32{ .5F, .5F, .5F, 1.F}
    };
    vmath::Mat4f32 projection;
    Camera camera;
    T params;
    bool shadow_casting;

    const void* data() const { return &params; }
    static constexpr std::size_t size() { return sizeof(T); }
};

using DirectionalLight = Light<DirectionalLightData>;
using PointLight = Light<PointLightData>;
using SpotLight = Light<SpotLightData>;

}

#endif