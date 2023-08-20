#ifndef VE001_MESHING_ENGINE_H
#define VE001_MESHING_ENGINE_H

#include <vmath/vmath.h>
#include <functional>

using namespace vmath;

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
        Vec3i32 chunk_size;
    };

    enum MeshingFace : u8 { 
        POS_X, NEG_X, 
        POS_Y, NEG_Y, 
        POS_Z, NEG_Z 
    };

    struct MeshedRegionDescriptor {
        MeshingFace face;
        Vec3i32 region_extent;
        Vec3i32 region_offset;
        Vec2i32 squashed_region_extent;
    };

private:
    Config config;

public:
    MeshingEngine(Config config);

    /**
     * @brief 
    */
    std::array<u32, 6> mesh(
        void* dst, 
        u32 offset, 
        u32 stride, 
        u32 face_stride, 
        Vec3f32 position, 
        const std::function<bool(i32, i32, i32)>& fn_state_query,
        const std::function<void(void*, MeshedRegionDescriptor)>& fn_write_quad
    );

private:

    u32 meshAxis(
        MeshingFace meshing_face, 
        void* face_dst, 
        u32 face_dst_max_size,
        u32 offset, 
        u32 stride, 
        Vec3f32 position, 
        const std::function<bool(i32, i32, i32)>& fn_state_query,
        const std::function<void(void*, MeshedRegionDescriptor)>& fn_write_quad
    );
};

}

#endif