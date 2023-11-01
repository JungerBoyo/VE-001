#ifndef VE001_WORLD_GRID_H
#define VE001_WORLD_GRID_H

#include <vmath/vmath.h>
#include <vector>
#include <numeric>
#include "chunk_id.h"
#include "chunk_pool.h"
#include "engine_context.h"
// #include "thread_pool.h"

#include "ringbuffer.h"

#include <voxel_terrain_generator/voxel_terrain_generator.h>

namespace ve001 {

struct WorldGrid {
    using VisibleChunkId = vmath::u32;
    static constexpr VisibleChunkId INVALID_VISIBLE_CHUNK_ID{ std::numeric_limits<vmath::u32>::max() };
    static constexpr vmath::u32 INVALID_VISIBLE_CHUNK_INDEX{ std::numeric_limits<vmath::u32>::max() };

    struct VisibleChunk {
        static constexpr vmath::u32 INVALID_NEIGHBOUR_INDEX{
            std::numeric_limits<vmath::u32>::max()
        };
        VisibleChunkId visible_chunk_id;
        ChunkId chunk_id;
        vmath::Vec3i32 position_in_chunks;
        vmath::u32 neighbours_count{ 0U };
        std::array<vmath::u32, 6> neighbours_indices{{
            INVALID_NEIGHBOUR_INDEX,
            INVALID_NEIGHBOUR_INDEX,
            INVALID_NEIGHBOUR_INDEX,
            INVALID_NEIGHBOUR_INDEX,
            INVALID_NEIGHBOUR_INDEX,
            INVALID_NEIGHBOUR_INDEX
        }};
    };
    struct ToAllocateChunk {
        std::future<std::optional<std::span<const vmath::u16>>> data;
        VisibleChunkId visible_chunk_id;
        std::optional<std::span<const vmath::u16>> ready_data{ std::nullopt };
    };

    const EngineContext& _engine_context;

    vmath::Vec3f32 _current_position;
    vmath::Vec3f32 _semi_axes;
    vmath::Vec3i32 _grid_size;

    VoxelTerrainGenerator _terrain_generator;
    RingBuffer<ToAllocateChunk> _to_allocate_chunks;

    std::vector<VisibleChunkId> _free_visible_chunk_ids;
    std::vector<vmath::u32> _visible_chunk_id_to_index;
    std::vector<VisibleChunk> _visible_chunks;

    std::vector<vmath::u32> _tmp_indices;

    ChunkPool _chunk_pool;

    WorldGrid(const EngineContext& engine_context, vmath::Vec3f32 world_size, vmath::Vec3f32 initial_position);
    void init();
    void update(vmath::Vec3f32 new_position);
    bool pollToAllocateChunks();

    bool validate();
    void deinit();
};

}

#endif