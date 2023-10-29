#ifndef VE001_NOISE_FUNC_H
#define VE001_NOISE_FUNC_H

#include <vmath/vmath.h>
#include <span>

namespace ve001 {
    struct NoiseFunc3D {
        vmath::u32 seed;
        NoiseFunc3D(vmath::u32 seed) : seed(seed) {}
        virtual vmath::f32 invoke(vmath::Vec3f32 point, vmath::u32 grid_density) = 0;
    };

    struct PerlinNoiseFunc3D : public NoiseFunc3D {
        PerlinNoiseFunc3D(vmath::u32 seed) : NoiseFunc3D(seed) {}
        vmath::f32 invoke(vmath::Vec3f32 point, vmath::u32 grid_density) override;
    };

    struct SimplexNoiseFunc3D : public NoiseFunc3D {
        SimplexNoiseFunc3D(vmath::u32 seed) : NoiseFunc3D(seed) {}
        vmath::f32 invoke(vmath::Vec3f32 point, vmath::u32 grid_density) override;
    };

    struct NoiseFunc2D {
        vmath::u32 seed;
        NoiseFunc2D(vmath::u32 seed) : seed(seed) {}
        virtual vmath::f32 invoke(vmath::Vec2f32 point, vmath::u32 grid_density) = 0;
    };

    struct PerlinNoiseFunc2D : public NoiseFunc2D {
        PerlinNoiseFunc2D(vmath::u32 seed) : NoiseFunc2D(seed) {}
        vmath::f32 invoke(vmath::Vec2f32 point, vmath::u32 grid_density) override;
    };
}

#endif