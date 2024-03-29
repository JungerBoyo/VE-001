#ifndef VE001_WORLD_GRID_H
#define VE001_WORLD_GRID_H

#include <vector>
#include <numeric>

#include <vmath/vmath.h>

#include "chunk_id.h"
#include "chunk_pool.h"
#include "engine_context.h"
#include "ringbuffer.h"
#include "chunk_data_streamer.h"

namespace ve001 {

/// @brief world grid is responsible for managing which chunk are visible. It calls 
/// chunk data streamer for new chunks of 3d voxel data and allocates them on chunk pool. It also
/// deallocates chunks becoming invisible from chunk pool.
struct WorldGrid {
    /// @brief id of visible chunk
    using VisibleChunkId = vmath::u32;
    static constexpr VisibleChunkId INVALID_VISIBLE_CHUNK_ID{ std::numeric_limits<vmath::u32>::max() };
    static constexpr vmath::u32 INVALID_VISIBLE_CHUNK_INDEX{ std::numeric_limits<vmath::u32>::max() };

    /// @brief visible chunk metadata structure
    struct VisibleChunk {
        static constexpr vmath::u32 INVALID_NEIGHBOUR_INDEX{
            std::numeric_limits<vmath::u32>::max()
        };
        /// @brief id of a chunk in the context of world grid
        VisibleChunkId visible_chunk_id;
        /// @brief id of a chunk in the context of chunk pool
        ChunkId chunk_id;
        /// @brief position of visible chunks in chunk size units
        vmath::Vec3i32 position_in_chunks;
        /// @brief number of visible chunks's neighbours 
        vmath::u32 neighbours_count{ 0U };
        /// @brief contains indices of neighbours in <_visible_chunks> array
        std::array<vmath::u32, 6> neighbours_indices{{
            INVALID_NEIGHBOUR_INDEX,
            INVALID_NEIGHBOUR_INDEX,
            INVALID_NEIGHBOUR_INDEX,
            INVALID_NEIGHBOUR_INDEX,
            INVALID_NEIGHBOUR_INDEX,
            INVALID_NEIGHBOUR_INDEX
        }};
    };
    /// @brief handle to chunk which is to be generated and than allocated
    struct ToAllocateChunk {
        /// @brief handle to data which will be generated in the future by
        /// chunk data streamer
        std::future<std::optional<std::span<const vmath::u16>>> data;
        /// @brief handle to visible chunk
        VisibleChunkId visible_chunk_id;
        /// @brief handle to generated data
        std::optional<std::span<const vmath::u16>> ready_data{ std::nullopt };
    };

    /// @brief engine context 
    const EngineContext& _engine_context;
    /// @brief maximum possible number of visible chunks
    vmath::u32 _max_visible_chunks;
    /// @brief current position
    vmath::Vec3f32 _current_position;
    /// @brief semi axes of the ellipsoid defining visible area
    vmath::Vec3f32 _semi_axes;
    /// @brief grid size, namely how big is the cuboid which
    /// contains ellipsoid in chunk size units
    vmath::Vec3i32 _grid_size;
    /// @brief chunk data streamer
    ChunkDataStreamer _chunk_data_streamer;
    /// @brief queue of chunks to generate and then allocate
    RingBuffer<ToAllocateChunk> _to_allocate_chunks;
    /// @brief available visible chunks' ids
    std::vector<VisibleChunkId> _free_visible_chunk_ids;
    /// @brief translates visible chunk id to index in visible chunks array
    std::vector<vmath::u32> _visible_chunk_id_to_index;
    /// @brief array of visible chunks. It is object pooled
    std::vector<VisibleChunk> _visible_chunks;
    /// @brief temporary indices which is a 3d grid of _grid_size. It holds neighbours indices written
    /// in the current update call. It exists as a fast reference to check if the neighbour was 
    /// already written by some other neighbour. It is cleared at the end of update call.
    std::vector<vmath::u32> _tmp_indices;

    ChunkPool _chunk_pool;
    
    /// @brief world grid constructor, it doesn't perform any opengl related initialization
    /// @param engine_context engine context
    /// @param world_size world size aka semi axes of an ellipsoid
    /// @param initial_position initial world grid continuous in space position (eg. camera initial position)
    /// @param chunk_data_streamer_threads_count number of <_chunk_data_streamer>'s threads count
    /// @param chunk_generator chunk generator which will be used by <_chunk_data_streamer>
    WorldGrid(
        EngineContext& engine_context, 
        vmath::Vec3f32 world_size, 
        vmath::Vec3f32 initial_position,
        vmath::u32 chunk_data_streamer_threads_count,
        std::unique_ptr<ChunkGenerator> chunk_generator
    ) noexcept;
    /// @brief performs later stage initialization eg. perform related initialization
    void init() noexcept;
    /// @brief updated the world grid based on the new position (eg. new camera position).
    /// Currently it is unsafe to supply position which more than 1 in chunk size units
    /// in any of the axes
    void update(vmath::Vec3f32 new_position) noexcept;
    /// @brief polls for the chunks which aren't yet generated by chunk data streamer and
    /// those which are generated but aren't yet allocated on the chunk pool. It is non-blocking
    /// @return true - there are yet chunks to be confirmed generated or allocated on the chunk pool
    bool pollToAllocateChunks() noexcept;
    /// @brief deinitializes all opengl related state (chunk pool)
    void deinit() noexcept;
};

}

#endif