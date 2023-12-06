#ifndef VE001_CHUNK_POOL_H
#define VE001_CHUNK_POOL_H

#include <vector>
#include <span>
#include <vmath/vmath_types.h>

#include "vertex.h"
#include "enums.h"
#include "ringbuffer.h"
#include "meshing_engine.h"
#include "engine_context.h"
#include "chunk_id.h"

namespace ve001 {

struct ChunkPool {

    /// This structure stores and manages gpu chunk data (meshes) allocation as well
    /// as cpu chunk data allocation (voxel states)

    ////////////////////////////
    ///      STRUCTURES      ///
    ////////////////////////////

    /// @brief structure stores drawing command data
    /// which is then stored in draw command buffer
    struct DrawElementsIndirectCmd {
        /// @brief number of indices
        vmath::u32 count;
        /// @brief number of instances
        vmath::u32 instance_count;
        /// @brief first index from which to start
        vmath::u32 first_index;
        /// @brief base vertex from which indices start to apply
        vmath::i32 base_vertex;
        /// @brief base instance from which to start
        vmath::u32 base_instance;

        /// @brief what is orientation of submesh drawn by this command
        Face orientation;
        /// @brief what is the chunk id to which the drawn by this command
        /// submesh belongs
        ChunkId chunk_id{ 0U };
    };

    /// @brief structure stores free chunk metadata
    struct FreeChunk {
        /// @brief free chunk id
        ChunkId chunk_id{ 0U };
        /// @brief free cpu region to allocate (derived from <_voxel_data>)
        std::span<vmath::u16> cpu_region;
    };

    /// @brief structure stores chunk metadata information
    struct Chunk {
        /// @brief chunk world space position 
        vmath::Vec3f32 position;
        /// @brief indicies of draw commands belonging to that chunk
        vmath::u32 draw_cmd_indices[6];
        /// @brief pointer to allocated cpu region for this chunk 
        std::span<vmath::u16> cpu_region;
        /// @brief unique id of this chunk
        ChunkId chunk_id{ 0U };
        /// @brief indicates if chunk is meshed and actively drawn 
        bool complete;
    };

    ////////////////////////////


    //////////////////////////
    ///     GL HANDLES     ///
    //////////////////////////

    /// @brief id of vbo storing all submeshes in regions
    vmath::u32 _vbo_id{ 0U };
    /// @brief id of ibo storing indices, it's the same for each submesh in each chunk
    /// it's size is always max_submesh_size
    vmath::u32 _ibo_id{ 0U };
    /// @brief id of vao describing vertex layout in vbo
    vmath::u32 _vao_id{ 0U };
    /// @brief id of dibo which is a handle to command buffer 
    /// which stores draw commands for all submeshes (reflects 
    /// submeshes stored in vbo)
    vmath::u32 _dibo_id{ 0U };
    void* _dibo_mapped_ptr{ nullptr };

    ///////////////////////////


    //////////////////////////////////
    ///     GPU SIDE VOXEL DATA    ///
    //////////////////////////////////

    /// @brief buffer of draw commands which draw submeshes stored in vbo
    std::vector<DrawElementsIndirectCmd> _draw_cmds;

    std::size_t _draw_cmds_parition_size{ 0UL };
    bool _draw_cmds_dirty{ false };
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

    /// @brief count of all submeshes
    vmath::i32 _submeshes_count{ 0 };
    
    //////////////////////////////////

    
    ////////////////////////////////////
    ///       METADATA (SHARED)      ///
    ////////////////////////////////////

    /// @brief count of all chunks in a pool
    vmath::u32 _chunks_count;
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
    EngineContext& _engine_context;

    /// @brief meshing engine of the chunk pool. It schedules meshing
    /// tasks on the GPU
    MeshingEngine _meshing_engine{ _engine_context, _chunks_count };

    ChunkPool(EngineContext& engine_context, vmath::u32 max_chunks) : 
        _chunks_count(max_chunks), _engine_context(engine_context) {}
    /// @brief initializes chunk pool
    /// @param max_chunks number of chunks in a pool
    void init() noexcept;
    /// @brief allocates chunk from _free_chunks
    /// @param voxel_write_data function writing voxel data to voxel data region (CPU)
    /// @param position position of the chunk
    /// @return allocated chunk id or UINT32_MAX if allocatation failed
    ChunkId allocateChunk(const std::function<void(void*)>& voxel_write_data, vmath::Vec3i32 position) noexcept;
    /// @brief allocates chunk from _free_chunks
    /// @param src voxel data
    /// @param position position of the chunk
    /// @return allocated chunk id or UINT32_MAX if allocatation failed
    ChunkId allocateChunk(std::span<const vmath::u16> src, vmath::Vec3i32 position) noexcept;
    /// @brief completes chunk eg. chunk starts to be drawn by the drawAll command 
    /// called by poll() function if chunk's mesh is finished
    /// @param result holds data from meshing_engine with which to update the chunk
    void completeChunk(MeshingEngine::Result result);
    /// @brief deallocates chunk
    /// @param chunk_id chunk's id to deallocate
    void deallocateChunk(ChunkId chunk_id) noexcept;
    /// @brief deallocates draw commands of the chunk. Called by deallocateChunk only
    /// if deallocated chunk is complete
    /// @param chunk_id chunk's id from which to deallocate draw commands
    void deallocateChunkDrawCommands(ChunkId chunk_id);
    /// @brief flag draw commands array as dirty to force memory reupload
    void forceCommandsDirty();
    /// @brief updates the state. Update draw command buffer binds vbo as vertex buffer, binds vao
    /// @param use_partition commands will be supplied based on last paritioning call (paritionDrawCmds)
    void update(bool use_partition);
    /// @brief draws all chunks
    /// @param use_partition number of draw commands will be based on last paritioning call (paritionDrawCmds) 
    void drawAll(bool use_partition);
    /// @brief recreates chunk pool based on the meshing result which caused overflow
    /// @param overflow_result meshing result which contains info about overflow
    void recreatePool(MeshingEngine::Result overflow_result);

    /// @brief polls for chunks that are meshed and are ready to be completed (one at a time)
    /// @return true if chunk was completed false otherwise
    bool poll();
    /// @brief deinitializes chunk pool
    void deinit();

    
    /// @brief function paritions the _draw_cmds based on <unary_op> setting the <_draw_cmds_parition_size> member
    /// @tparam ...Args types of aux arguments to pass to unary_op function
    /// @param unary_op unary operation which is criterion based on which the commands are partitioned
    /// @param use_last_partition If true then the previous parition will be paritioned again
    /// @param args Aux arguments to pass to unary_op function
    template<typename ...Args> 
    void partitionDrawCommands(bool(*unary_op)(Face orientation, vmath::Vec3f32 position, Args... args), bool use_last_partition, Args... args) {
        std::size_t begin{ 0UL };
        std::size_t end{ use_last_partition ? _draw_cmds_parition_size - 1UL : _draw_cmds.size() - 1UL };

        while(true) {
            while(begin < _draw_cmds.size() && unary_op(_draw_cmds[begin].orientation, _chunks[_chunk_id_to_index[_draw_cmds[begin].chunk_id]].position, args...)) { ++begin; }
            while(end != std::numeric_limits<std::size_t>::max() && !unary_op(_draw_cmds[end].orientation, _chunks[_chunk_id_to_index[_draw_cmds[end].chunk_id]].position, args...)) { --end; }
            
            if (end == std::numeric_limits<std::size_t>::max() || begin >= end) {
                break;
            }

            _chunks[_chunk_id_to_index[_draw_cmds[begin].chunk_id]].
                draw_cmd_indices[_draw_cmds[begin].orientation] = end;
            
            _chunks[_chunk_id_to_index[_draw_cmds[end].chunk_id]].
                draw_cmd_indices[_draw_cmds[end].orientation] = begin;
            
            std::swap(_draw_cmds[end], _draw_cmds[begin]);

            --end;
            ++begin;
        }
        _draw_cmds_parition_size = begin;
        _draw_cmds_dirty = true;
    }
};

}

#endif