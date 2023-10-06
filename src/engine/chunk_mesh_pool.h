#ifndef VE001_CHUNK_MESH_POOL_H
#define VE001_CHUNK_MESH_POOL_H

#include <vmath/vmath.h>
#include "ringbuffer.h"
#include "errors.h"
#include "enums.h"

#include <array>

namespace ve001 {

struct ChunkMeshPool {
    struct Config {
        vmath::Vec3i32 chunk_dimensions;
        vmath::i32 max_chunks;
        vmath::i32 vertex_size;
        void (*vertex_layout_config)(vmath::u32 vao, vmath::u32 vbo);
    };

    struct SubmeshData {
        void* ptr{ nullptr };
        vmath::i32 vertex_count{ 0 };
        Face orientation;
    };

    struct ChunkData {
        SubmeshData submesh_data[6];
        vmath::i32 size;
    };

private:
    static constexpr vmath::u32 INVALID_CHUNK_INDEX{ std::numeric_limits<vmath::u32>::max() };

    struct FreeChunk {
        vmath::u32 chunk_id{ 0U };
        void* region{ nullptr };
    };

    struct ChunkMetadata {
        vmath::Vec3f32 position;
        vmath::u32 draw_cmd_indices[6];
        void* region;
        vmath::u32 chunk_id{ 0U };
        bool aquired{ false };
    };

    struct DrawArraysIndirectCmd {
        vmath::u32 count;
        vmath::u32 instance_count;
        vmath::u32 first;
        vmath::u32 base_instance;
        
        Face orientation;
        vmath::u32 chunk_id{ 0U };
    };

    vmath::u32 _vbo_id{ 0U };
    vmath::u32 _vao_id{ 0U };
    vmath::u32 _dibo_id{ 0U };

    vmath::i32 _max_chunk_size{ 0 };
    vmath::i32 _chunks_count{ 0 };

    vmath::i32 _max_submesh_size{ 0 };
    vmath::i32 _submeshes_count{ 0 };

    vmath::i32 _vertex_size{ 0 };

    void* _vbo_ptr{ nullptr };

    RingBuffer<FreeChunk> _free_chunks;

    std::vector<DrawArraysIndirectCmd> _draw_cmds;

    std::vector<ChunkMetadata> _chunk_metadata;
    std::vector<vmath::u32> _chunk_id_to_index;
public:
    ChunkMeshPool() = default;

    Error init(Config config) noexcept;
    Error allocateChunk(vmath::u32& chunk_id, ChunkData chunk_data, vmath::Vec3f32 position) noexcept;
    Error allocateEmptyChunk(vmath::u32& chunk_id, vmath::Vec3f32 position) noexcept;
    Error writeChunk(vmath::u32 chunk_id, ChunkData chunk_data) noexcept;
    Error updateChunkSubmeshVertexCounts(vmath::u32 chunk_id, std::array<vmath::u32, 6> counts) noexcept;
    Error aquireChunkWritePtr(vmath::u32 chunk_id, void*& dst) noexcept;
    Error freeChunkWritePtr(vmath::u32 chunk_id) noexcept;

    Error deallocateChunk(vmath::u32 chunk_id) noexcept;


    const vmath::u32 submeshStride() const { return _max_submesh_size; };

    void update() noexcept;
    void drawAll() noexcept;

    void deinit() noexcept;

private:
    void deallocateChunkDrawCmds(const ChunkMetadata& chunk_metadata);
};

}

#endif