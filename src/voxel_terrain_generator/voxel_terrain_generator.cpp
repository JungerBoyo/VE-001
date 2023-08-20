#include "voxel_terrain_generator.h"

#include <cstring>
#include <cmath>

using namespace vmath;
using namespace ve001;

VoxelTerrainGenerator::VoxelTerrainGenerator(Config config) 
    : config(config),
      fn_write_value(config.quantize_values == 0U ?
        std::function([](void* dst, f32 value) {
            const auto normalized_value{ (value + 1.F)/2.F };
            std::memcpy(dst, static_cast<const void*>(&normalized_value), sizeof(f32));
        }) :
        std::function([quantize_values = config.quantize_values, size = config.quantized_value_size](void* dst, f32 value) {
            const auto quantized_value{ 
                static_cast<u32>(std::roundf(((value + 1.F)/2.F) * static_cast<f32>(quantize_values)))
            };
            std::memcpy(dst, static_cast<const void*>(&quantized_value), size);
        })
    ) {}

void VoxelTerrainGenerator::init() {
    if (config.noise_type == Config::NoiseType::PERLIN) {
        noise_func_3d = std::make_unique<PerlinNoiseFunc3D>(config.seed);
    } else {
        noise_func_3d = std::make_unique<SimplexNoiseFunc3D>(config.seed);
    }
}

void VoxelTerrainGenerator::next(void* dst, u32 offset, u32 stride, Vec3i32 terrain_step) {
    const auto p0 = Vec3i32::mul(current_location, config.terrain_size);
    const auto p1 = Vec3i32::add(p0, config.terrain_size);

    current_location = Vec3i32::add(current_location, terrain_step);

    u8* dst_u8 = static_cast<u8*>(dst);
    dst_u8 += offset;

    if (stride == 0U) { 
        stride = config.quantize_values > 0U ? config.quantized_value_size : sizeof(f32); 
    }

    for (i32 z{ p0[2] }; z < p1[2]; ++z) {
        for (i32 y{ p0[1] }; y < p1[1]; ++y) {
            for (i32 x{ p0[0] }; x < p1[0]; ++x) {
                const auto value = noise_func_3d->invoke(Vec3f32{
                    static_cast<f32>(x),
                    static_cast<f32>(y),
                    static_cast<f32>(z)
                }, config.terrain_density);
                fn_write_value(static_cast<void*>(dst_u8), value);
                dst_u8 += stride;
            }
        }
    }
}

void VoxelTerrainGenerator::deinit() {
    
}