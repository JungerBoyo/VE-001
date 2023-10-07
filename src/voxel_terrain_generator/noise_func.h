#ifndef VE001_NOISE_FUNC_H
#define VE001_NOISE_FUNC_H

#include <vmath/vmath.h>
#include <span>

using namespace vmath;

namespace ve001 {
    struct NoiseFunc3D {
        u32 seed;
        NoiseFunc3D(u32 seed) : seed(seed) {}
        virtual f32 invoke(Vec3f32 point, u32 grid_density) = 0;
    };

    struct PerlinNoiseFunc3D : public NoiseFunc3D {
        PerlinNoiseFunc3D(u32 seed) : NoiseFunc3D(seed) {}
        f32 invoke(Vec3f32 point, u32 grid_density) override;
    };

    struct SimplexNoiseFunc3D : public NoiseFunc3D {
        SimplexNoiseFunc3D(u32 seed) : NoiseFunc3D(seed) {}
        f32 invoke(Vec3f32 point, u32 grid_density) override;
    };

    struct NoiseFunc2D {
        u32 seed;
        NoiseFunc2D(u32 seed) : seed(seed) {}
        virtual f32 invoke(Vec2f32 point, u32 grid_density) = 0;
    };

    struct PerlinNoiseFunc2D : public NoiseFunc2D {
        PerlinNoiseFunc2D(u32 seed) : NoiseFunc2D(seed) {}
        f32 invoke(Vec2f32 point, u32 grid_density) override;
    };
}

#endif