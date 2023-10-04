#ifndef VE001_MESHING_ENGINE_H
#define VE001_MESHING_ENGINE_H

#include <vmath/vmath.h>
#include <functional>

#include "enums.h"

namespace ve001 {

struct MeshingEngine {
    struct Config {
        enum class Executor { CPU, GPU };
        /**
         * @brief CPU or GPU
        */
        Executor executor;
        /**
         * @brief size of a chunk
        */
        vmath::Vec3i32 chunk_size;
    };

    struct MeshedRegionDescriptor {
        Face face;
        vmath::Vec3i32 region_extent;
        vmath::Vec3i32 region_offset;
        vmath::Vec2i32 squashed_region_extent;
    };

private:
    Config config;

public:
    MeshingEngine(Config config);

    /**
     * @brief 
    */
    std::array<vmath::u32, 6> mesh(
        void* dst, 
        vmath::u32 offset, 
        vmath::u32 stride, 
        vmath::u32 face_stride, 
        vmath::Vec3i32 position, 
        const std::function<bool(vmath::i32, vmath::i32, vmath::i32)>& fn_state_query,
        const std::function<vmath::u32(void*, MeshedRegionDescriptor)>& fn_write_quad
    );

private:
    vmath::u32 meshAxis(
        Face meshing_face, 
        void* face_dst, 
        vmath::u32 face_dst_max_size,
        vmath::u32 offset, 
        vmath::u32 stride, 
        vmath::Vec3i32 position, 
        const std::function<bool(vmath::i32, vmath::i32, vmath::i32)>& fn_state_query,
        const std::function<vmath::u32(void*, MeshedRegionDescriptor)>& fn_write_quad
    );
};

}

#endif