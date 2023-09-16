#ifndef VE001_CHUNK_MESH_POOL_H
#define VE001_CHUNK_MESH_POOL_H

#include <vmath/vmath.h>
#include "ringbuffer.h"
#include "errors.h"
#include "enums.h"

#include <array>

using namespace vmath;

namespace ve001 {

struct ChunkMeshPool {
    struct Config {
        Vec3i32 chunk_dimensions;
        i32 max_chunks;
        i32 vertex_size;
        void (*vertex_layout_config)(u32 vao, u32 vbo);
    };

    struct SubmeshData {
        void* ptr{ nullptr };
        i32 vertex_count{ 0 };
        Face orientation;
    };

    struct ChunkData {
        SubmeshData submesh_data[6];
        i32 size;
    };

private:
    static constexpr u32 INVALID_CHUNK_INDEX{ std::numeric_limits<u32>::max() };

    struct FreeChunk {
        u32 chunk_id{ 0U };
        void* region{ nullptr };
    };

    struct ChunkMetadata {
        Vec3f32 position;
        u32 draw_cmd_indices[6];
        void* region;
        u32 chunk_id{ 0U };
        bool aquired{ false };
    };

    struct DrawArraysIndirectCmd {
        u32 count;
        u32 instance_count;
        u32 first;
        u32 base_instance;
        
        Face orientation;
        u32 chunk_id{ 0U };
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

    RingBuffer<FreeChunk> _free_chunks;

    std::vector<DrawArraysIndirectCmd> _draw_cmds;

    std::vector<ChunkMetadata> _chunk_metadata;
    std::vector<u32> _chunk_id_to_index;
public:
    ChunkMeshPool() = default;

    Error init(Config config) noexcept;
    Error allocateChunk(u32& chunk_id, ChunkData chunk_data, Vec3f32 position) noexcept;
    Error allocateEmptyChunk(u32& chunk_id, Vec3f32 position) noexcept;
    Error writeChunk(u32 chunk_id, ChunkData chunk_data) noexcept;
    Error updateChunkSubmeshVertexCounts(u32 chunk_id, std::array<u32, 6> counts) noexcept;
    Error aquireChunkWritePtr(u32 chunk_id, void*& dst) noexcept;
    Error freeChunkWritePtr(u32 chunk_id) noexcept;

    Error deallocateChunk(u32 chunk_id) noexcept;


    const u32 submeshStride() const { return _max_submesh_size; };


    void drawAll() noexcept;

    void deinit() noexcept;

private:
    void deallocateChunkDrawCmds(const ChunkMetadata& chunk_metadata);
};

}

#endif