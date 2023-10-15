#ifndef VE001_MESHING_ENGINE_2_H
#define VE001_MESHING_ENGINE_2_H

#include <vmath/vmath.h>
#include <functional>
#include <span>
#include <array>

#include "enums.h"
#include "gpu_buffer.h"
#include "ringbuffer.h"

namespace ve001 {

struct MeshingEngine {
    /// @brief Descriptor of meshing, it maps to the ubo of binding id 2 in
    /// greedy meshing shader
    struct Descriptor {
        /// @brief offsets of submeshes (constant)
        vmath::u32 vbo_offsets[6];
        /// @brief position changes per meshing command (mutable)
        vmath::Vec3i32 chunk_position;
        /// @brief size of a chunk (constant)
        vmath::Vec3i32 chunk_size;
    };

    /// @brief holds temporary data of current meshing command execution
    struct Temp {
        /// @brief written vertex counts by the current meshing command execution
        /// (depending on <_chunk_size> there could be a need to invoke meshing
        /// few times per chunk). It is written and read in the shader.
        vmath::u32 written_vertex_counts[6] = {0};
        /// @brief axes steps of the current meshing command execution
        /// (depending on <_chunk_size> there could be a need to invoke meshing
        /// few times per chunk). It is written and read in the shader.
        vmath::u32 axes_steps[6] = {0};
    };

    /// !!! ///
    // Vertex vbo[] <-- implicit (range bound VBO mesh buffer from ChunkPool) see 
    /////////////// greedy meshing shader

    /// @brief command descripting meshing execution of a single chunk
    struct Command {
        /// @brief id of chunk meshed by command
        u32 chunk_id; 
        /// @brief position of the chunk
        Vec3i32 chunk_position;
        /// @brief pointer to voxel data to be issued before meshing starts
        std::span<u16> voxel_data;
        /// @brief offset into vbo containing a mesh (maps to vbo in ChunkPool)
        u32 vbo_offset;
        /// @brief size of vbo
        u32 vbo_size;
        /// @brief gl fence for which to wait in case the command is active one
        /// also indicates !!!if the command is initialized (nullptr here if not)!!!
        void* fence;
        /// @brief axis progress keeps track ofa how many planes on each axes were meshed
        i32 axis_progress;
    };

    /// @brief is an interface and holds completed command data 
    struct Future {
        /// @brief indentifier of a succesfully meshed chunk (maps to
        /// chunk id in ChunkPool)
        vmath::u32 chunk_id;
        /// @brief written vertices count *per face* indexed with ve001::Face enum. 
        /// Needed to determine how much vertices to render
        std::array<vmath::u32, 6> written_vertices_counts;
    };

    /// @brief it maps to local_size_x attribute in greedy meshing compute shader
    static constexpr i32 AXIS_PROGRESS_STEP{ 32 };

    /// @brief sizeof ssbo voxel data
    vmath::u32 _ssbo_voxel_data_size;
    /// @brief id of buffer holding voxel data for subsequent meshing command execution
    /// (WRITE_ONLY, PERSISTENT, COHERENT)
    vmath::u32 _ssbo_voxel_data_id{ 0U };
    /// @brief pointer to mapped _ssbo_voxel_data_id (persistent)
    void* _ssbo_voxel_data_ptr{ nullptr };
    /// @brief gpu buffer for <MeshingDescriptor> data
    GPUBuffer _ubo_meshing_descriptor{ sizeof(MeshingDescriptor) };
    /// @brief gpu buffer for <MeshingTemp> data
    GPUBuffer _ssbo_meshing_temp{ sizeof(MeshingTemp) };
    /// @brief id of vbo holding meshes (the same vbo as in ChunkPool)
    vmath::u32 _vbo_id{ 0U };

    RingBuffer<Command> _commands;
    Command _active_command;

    /// @brief chunk size/resolution
    vmath::Vec3i32 _chunk_size;

    MeshingEngine(vmath::Vec3i32 chunk_size) 
        : _chunk_size(chunk_size),
          _ssbo_voxel_data_size(chunk_size[0] * chunk_size[1] * chunk_size[2] * sizeof(u16)) {}

    void init(vmath::u32 vbo_id);

    /// @brief issues meshing command to the engine
    /// @param chunk_id id of processed chunk. Maps to chunk ids in ChunkPool
    /// @param chunk_position chunk position
    /// @param voxel_data pointer to voxel_data based on which the meshing will take place
    /// @param vbo_offset offset into VBO from ChunkPool (needed for range binding VBO)
    /// @param vbo_size size of VBO from <vbo_offset> from ChunkPool (needed for range binding VBO)
    void issueMeshingCommand(
        vmath::u32 chunk_id,
        vmath::Vec3i32 chunk_position,
        std::span<vmath::u16> voxel_data,
        vmath::u32 vbo_offset,
        vmath::u32 vbo_size 
    );

    /// @brief Function polls for the result from next command. It isn't waiting (the call
    // is non blocking), only checks once.
    /// @param future the future variable to which the function the write into
    /// @return true if valid value was written into the <future> param false if not
    bool pollMeshingCommand(Future& future);

    /// @brief executes command meaning dispatches meshing based on parameters 
    /// in the <command>. It is first execution so the data is passed to the gpu here
    /// @param command command to be executed
    void firstCommandExec(Command& command);

    /// @brief executes command meaning dispatches meshing based on parameters 
    /// in the <command>. It is subsequent command execution so the per command data
    /// isn't allocated.
    /// @param command command to be executed
    void subsequentCommandExec(Command& command);

    void deinit();
};

}

#endif