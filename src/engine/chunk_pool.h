#ifndef VE001_CHUNK_POOL_H
#define VE001_CHUNK_POOL_H

#include <vector>
#include <span>
#include <vmath/vmath_types.h>

#include "vertex.h"
#include "enums.h"
#include "ringbuffer.h"
#include "meshing_engine2.h"
#include "engine_context.h"

namespace ve001 {

struct ChunkPool {

    /// This structure stores and manages gpu chunk data (meshes) allocation as well
    /// as cpu chunk data allocation (voxel states)

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
        /// @brief free gpu region to allocate offset into vbo_id 
        vmath::u64 gpu_region_offset{ 0U };
        /// @brief free cpu region to allocate (derived from <_voxel_data>)
        std::span<vmath::u16> cpu_region;
    };

    /// @brief structure stores chunk metadata information
    struct Chunk {
        /// @brief chunk world space position 
        vmath::Vec3i32 position;
        /// @brief indicies of draw commands belonging to that chunk
        vmath::u32 draw_cmd_indices[6];
        /// @brief allocated gpu region offset into vbo_id 
        vmath::u64 gpu_region_offset{ 0U };
        /// @brief pointer to allocated cpu region for this chunk 
        std::span<vmath::u16> cpu_region;
        /// @brief unique id of this chunk
        vmath::u32 chunk_id{ 0U };
        /// @brief indicates if chunk is meshed and actively drawn 
        bool complete;
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
    /// @brief count of all submeshes
    vmath::i32 _submeshes_count{ 0 };
    
    //////////////////////////////////

    
    ////////////////////////////////////
    ///       METADATA (SHARED)      ///
    ////////////////////////////////////

    /// @brief count of all chunks in a pool
    vmath::i32 _chunks_count;
    /// @brief helper array translating chunk_id to its
    /// index in <_chunks>
    std::vector<vmath::u32> _chunk_id_to_index;
    /// @brief designates invalid index in <_chunk_id_to_index> array
    static constexpr vmath::u32 INVALID_CHUNK_INDEX { std::numeric_limits<vmath::u32>::max() };
    /// @brief free chunks which can be allocated
    RingBuffer<FreeChunk> _free_chunks;
    /// @brief used chunks
    std::vector<Chunk> _chunks;

    ////////////////////////////////////


    ////////////////////////////////////////
    ///       METADATA (DEBUG/INFO)      ///
    ////////////////////////////////////////

    /// @brief gpu memory usage in bytes (mesh)
    vmath::u64 gpu_memory_usage{ 0UL };
    /// @brief cpu memory usage in bytes (voxel values)
    vmath::u64 cpu_memory_usage{ 0UL };    
    
    ////////////////////////////////////////

    /// @brief context holds common data to all engine components
    const EngineContext& _engine_context;

    /// @brief meshing engine of the chunk pool. It schedules meshing
    /// tasks on the GPU
    MeshingEngine2 _meshing_engine{ _engine_context };

    ChunkPool(const EngineContext& engine_context) : _engine_context(engine_context) {}
    /// @brief initializes chunk pool
    /// @param max_chunks number of chunks in a pool
    void init(vmath::i32 max_chunks) noexcept;
    /// @brief allocates chunk from _free_chunks
    /// @param voxel_write_data function writing voxel data to voxel data region (CPU)
    /// @param position position of the chunk
    /// @return allocated chunk id or UINT32_MAX if allocatation failed
    vmath::u32 allocateChunk(const std::function<void(void*)>& voxel_write_data, vmath::Vec3i32 position) noexcept;
    /// @brief allocates chunk from _free_chunks
    /// @param src voxel data
    /// @param position position of the chunk
    /// @return allocated chunk id or UINT32_MAX if allocatation failed
    vmath::u32 allocateChunk(std::span<const vmath::u16> src, vmath::Vec3i32 position) noexcept;
    /// @brief completes chunk eg. chunk starts to be drawn by the drawAll command 
    /// called by poll() function if chunk's mesh is finished
    /// @param future holds data from meshing_engine with which to update the chunk
    void completeChunk(MeshingEngine2::Future future);
    /// @brief deallocates chunk
    /// @param chunk_id chunk's id to deallocate
    void deallocateChunk(vmath::u32 chunk_id) noexcept;
    /// @brief deallocates draw commands of the chunk. Called by deallocateChunk only
    /// if deallocated chunk is complete
    /// @param chunk reference to chunk from which to deallocate draw commands
    void deallocateChunkDrawCommands(const Chunk& chunk);
    /// @brief updates the state. Update draw command buffer binds vbo as vertex buffer, binds vao
    void update();
    /// @brief draws all chunks
    void drawAll();
    /// @brief polls for chunks that are meshed and are ready to be completed (one at a time)
    /// @return true if chunk was completed false otherwise
    bool poll();
    /// @brief deinitializes chunk pool
    void deinit();
};

}

#endif