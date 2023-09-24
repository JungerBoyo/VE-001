#ifndef VE001_LIGHT_H
#define VE001_LIGHT_H

#include <vmath/vmath.h>

#include "camera.h"

using namespace vmath;

namespace ve001 {

struct Light {
    static constexpr Mat4f32 DEPTH_BIAS_MATRIX {
        Vec4f32{ .5F, 0.F, 0.F, 0.F},
        Vec4f32{ 0.F, .5F, 0.F, 0.F},
        Vec4f32{ 0.F, 0.F, .5F, 0.F},
        Vec4f32{ .5F, .5F, .5F, 1.F}
    };

    enum class Type : u32 {
        DIRECTIONAL,
        POINT,
        SPOT
    };
    
    Type type;

    Camera camera;
    Vec3f32 color;

    Vec3f32 ambient;
    Vec3f32 diffuse;
    Vec3f32 specular;

    f32 attenuation_constant;
    f32 attenuation_linear;
    f32 attenuation_quadratic;

    f32 inner_cut_off;
    f32 outer_cut_off;

    bool shadow_casting;
}; 

}

#endif