#ifndef VE001_CHUNK_POOL_H
#define VE001_CHUNK_POOL_H

#include <vector>
#include <vmath/vmath_types.h>

#include "vertex.h"
#include "enums.h"
#include "ringbuffer.h"

namespace ve001 {

/// @brief Structure stores and manages gpu chunk data (meshes) allocation as well
/// as cpu chunk data allocation (voxel states)
struct ChunkPool {
    ////////////////////////////
    ///      STRUCTURES      ///
    ////////////////////////////

    /// @brief structure stores drawing command data
    /// which is then stored in draw command buffer
    struct DrawArraysIndirectCmd {
        /// @brief number of verticies
        vmath::u32 count;
        /// @brief number of instances
        vmath::u32 instance_count;
        /// @brief first vertex from which to start
        vmath::u32 first;
        /// @brief base instance from which to start
        vmath::u32 base_instance;

        /// @brief what is orientation of submesh drawn by this command
        Face orientation;
        /// @brief what is the chunk id to which the drawn by this command
        /// submesh belongs
        vmath::u32 chunk_id{ 0U };
    };

    /// @brief structure stores free chunk metadata
    struct FreeChunk {
        /// @brief free chunk id
        vmath::u32 chunk_id{ 0U };
        /// @brief free gpu region to allocate (derived from <_vbo_ptr>)
        void* gpu_region{ nullptr };
        /// @brief free cpu region to allocate (derived from <_voxel_data>)
        void* cpu_region{ nullptr };
    };

    /// @brief structure stores chunk metadata information
    struct Chunk {
        /// @brief chunk world space position 
        vmath::Vec3f32 position;
        /// @brief indicies of draw commands belonging to that chunk
        vmath::u32 draw_cmd_indices[6];
        /// @brief pointer to allocated gpu region for this chunk 
        void* gpu_region;
        /// @brief pointer to allocated cpu region for this chunk 
        void* cpu_region;
        /// @brief unique id of this chunk
        vmath::u32 chunk_id{ 0U };
    };
    ////////////////////////////


    //////////////////////////
    ///     GL HANDLES     ///
    //////////////////////////

    /// @brief id of vbo storing all submeshes in regions
    vmath::u32 _vbo_id{ 0U };
    /// @brief id of vao describing vertex layout in vbo
    vmath::u32 _vao_id{ 0U };
    /// @brief id of dibo which is a handle to command buffer 
    /// which stores draw commands for all submeshes (reflects 
    /// submeshes stored in vbo)
    vmath::u32 _dibo_id{ 0U };
    ///////////////////////////


    //////////////////////////////////
    ///     GPU SIDE VOXEL DATA    ///
    //////////////////////////////////

    /// @brief pointer to mapped vbo storing mesh data. WRITE ONLY, 
    /// PERSISTED (no need for remapping) AND COHERENT (no need for manual sync)
    void* _vbo_ptr{ nullptr };
    /// @brief buffer of draw commands which draw submeshes stored in vbo
    std::vector<DrawArraysIndirectCmd> _draw_cmds;
    ///////////////////////////

      
    //////////////////////////////////
    ///     CPU SIDE VOXEL DATA    ///
    //////////////////////////////////

    /// @brief is divided into regions, reflects meshes (stored in vbo) in voxel data
    /// (cpu side)
    std::vector<vmath::u16> _voxel_data;
    //////////////////////////////////


    //////////////////////////////////
    ///       METADATA (MESH)      ///
    //////////////////////////////////

    /// @brief vertex size position + texcoord (24 bytes) 
    static constexpr vmath::i32 VERTEX_SIZE{ sizeof(Vertex) };
    /// @brief maximum possible mesh size of a single chunk after greedy meshing
    vmath::i32 _max_mesh_chunk_size{ 0 };
    /// @brief maximum possible mesh size of a single chunk's side after greedy meshing
    vmath::i32 _max_submesh_size{ 0 };
    /// @brief count of all submeshes
    vmath::i32 _submeshes_count{ 0 };
    //////////////////////////////////

    
    ////////////////////////////////////
    ///       METADATA (SHARED)      ///
    ////////////////////////////////////

    /// @brief count of all chunks in a pool
    vmath::i32 _chunks_count;
    /// @brief helper array translating chunk_id to its
    /// index in <TODO>
    std::vector<vmath::u32> _chunk_id_to_index;
    /// @brief designates invalid index in <_chunk_id_to_index> array
    static constexpr vmath::u32 INVALID_CHUNK_INDEX { std::numeric_limits<vmath::u32>::max() };
    /// @brief free chunks which can be allocated
    RingBuffer<FreeChunk> _free_chunks;
    /// @brief used chunks
    std::vector<Chunk> _chunks;
    ////////////////////////////////////

    ChunkPool() = default;

    void init(vmath::Vec3i32 chunk_dimensions, vmath::i32 max_chunks) noexcept;
    void deinit() noexcept;
};

}

#endif