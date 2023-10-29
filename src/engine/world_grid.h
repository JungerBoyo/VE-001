#ifndef VE001_WORLD_GRID_H
#define VE001_WORLD_GRID_H

#include <vmath/vmath.h>
#include <vector>
#include <numeric>
#include "chunk_id.h"
#include "chunk_pool.h"
#include "engine_context.h"
#include <voxel_terrain_generator/voxel_terrain_generator.h>

namespace ve001 {

struct WorldGrid {
    struct VisibleChunk {
        static constexpr vmath::u32 INVALID_NEIGHBOUR_INDEX{
            std::numeric_limits<vmath::u32>::max()
        };

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
    const EngineContext& _engine_context;

    vmath::Vec3f32 _current_position;
    vmath::Vec3f32 _semi_axes;
    vmath::Vec3i32 _grid_size;

    VoxelTerrainGenerator _terrain_generator;
    std::vector<vmath::u16> _noise;

    std::vector<vmath::Vec3i32> _to_allocate_chunks;
    std::vector<VisibleChunk> _visible_chunks;
    std::vector<vmath::u32> _tmp_indices;

    ChunkPool _chunk_pool;

    WorldGrid(const EngineContext& engine_context, vmath::Vec3f32 world_size, vmath::Vec3f32 initial_position);
    void init();
    void update(vmath::Vec3f32 new_position);

    bool validate();
    void deinit();
};

}

#endif