#ifndef VE001_TERRAIN_GENERATOR_H
#define VE001_TERRAIN_GENERATOR_H

#include <memory>
#include <functional>

#include <vmath/vmath.h>

#include "noise_func.h"

namespace ve001 {

struct VoxelTerrainGenerator {
    struct Config {
        // enum class Executor {
        //     CPU,
        //     GPU
        // };
        enum class NoiseType {
            PERLIN,
            SIMPLEX
        };
        
        /**
         * @brief Size of single piece of terrain
         * generated per iteration
        */
        vmath::Vec3i32 terrain_size;
        /**
         * @brief Since generator uses simplex/perlin noise,
         * this argument configures how many voxels
         * there are per simplex/perlin grid cell.
        */
        vmath::u32 terrain_density;
        /**
         * @brief CPU or GPU
        */
        // Executor executor;
        /**
         * @brief Type of noise used for terrain generation
        */
        NoiseType noise_type;
        /**
         * @brief if this value is 0 all gradient vectors in
         * noise grid are purely random. If > 0 then it specifies
         * number of vector in multiples of 8
        */
        // u32 unique_gradient_vectors_density;
        /**
         * @brief Optionally height map can be generated
         * before, which when applied to generated 
         * terrain's random values modifies them
         * in such a way that if value's coordinate's 
         * y value is contained within height then
         * value is attenuated otherwise it is impaired
        */
        // bool saturate_with_height_map;
        /**
         * @brief Generated terrain can be either floating point
         * if this value equals 0, or in discrete integer values
         * if this value is > 0. When > 0, then it specifies
         * max number of states to which to translate to.
         * (useful when generating eg. texture index).
        */
        vmath::u32 quantize_values;
        /**
         * @brief size in bytes of quatized value if <quantize_values> == 0U
         * it has no effect since values are floating point. Possible values 1, 2, 3 and 4
        */
        vmath::u32 quantized_value_size;
        /**
         * @brief seed to generate random gradients
        */
        vmath::u32 seed;
    };

private:
    Config config;
    std::unique_ptr<NoiseFunc3D> noise_func_3d{ nullptr };
    std::unique_ptr<NoiseFunc2D> noise_func_2d{ nullptr };
    // Vec3i32 current_location{ 0 };
    std::function<void(void*, vmath::f32)> fn_write_value;

public:    
    VoxelTerrainGenerator(Config config);
    /**
     * @brief generates next terrain chunk
     * @param dst generated data is written here
     * @param offset offset in bytes to the dst buffer
     * @param stride in bytes, if step == 0 assuming tighly packed data,
     * otherwise it specifies the relative position of generated values in dst buffer
     * (eg. if values in the buffer have interleaved layout)
     * @param chunk_position discrete position of the chunk in chunk extents
    */
    void next(void* dst, vmath::u32 offset, vmath::u32 stride, vmath::Vec3i32 chunk_position);
};

}

#endif