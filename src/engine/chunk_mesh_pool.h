#ifndef VE001_CHUNK_MESH_POOL_H
#define VE001_CHUNK_MESH_POOL_H

#include <vmath/vmath.h>
#include "ringbuffer.h"
#include "errors.h"

using namespace vmath;

namespace ve001 {

struct ChunkMeshPool {
    struct Config {
        Vec3i32 chunk_dimensions;
        i32 max_chunks;
        i32 vertex_size;
        void (*vertex_layout_config)(u32 vao, u32 vbo);
    };

    enum SubmeshOrientation : u32 {
        X_POS, X_NEG, Y_POS, Y_NEG, Z_POS, Z_NEG
    };
    
    struct SubmeshData {
        void* ptr{ nullptr };
        i32 vertex_count{ 0 };
        SubmeshOrientation orientation;
    };

    struct ChunkData {
        SubmeshData submesh_data[6];
        i32 size;
    };

private:

    struct ChunkMetadata {
        Vec3f32 position;
        u32 draw_cmd_indices[6];
        void* region;
    };

    struct DrawArraysIndirectCmd {
        u32 count;
        u32 instance_count;
        u32 first;
        u32 base_instance;
        
        SubmeshOrientation orientation;
        u32 chunk_index{ 0U };
    };

    u32 _vbo_id{ 0U };
    u32 _vao_id{ 0U };
    u32 _dibo_id{ 0U };

    i32 _max_chunk_size{ 0 };
    i32 _chunks_count{ 0 };

    i32 _max_submesh_size{ 0 };
    i32 _submeshes_count{ 0 };

    i32 _vertex_size{ 0 };

    void* _vbo_ptr{ nullptr };
    RingBuffer<void*> _free_pool_regions;

    std::vector<DrawArraysIndirectCmd> _draw_cmds;

    std::vector<ChunkMetadata> _chunk_metadata;
    // i32 _submeshes_scheduled_for_draw_count{ 0 };

public:
    ChunkMeshPool() = default;

    Error init(Config config) noexcept;
    Error allocateChunk(u32& chunk_id, ChunkData chunk_data, Vec3f32 position) noexcept;
    Error deallocateChunk(u32 chunk_id) noexcept;

    void drawAll() noexcept;

    void deinit() noexcept;
};

}

#endif