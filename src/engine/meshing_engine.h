#ifndef VE001_MESHING_ENGINE_H
#define VE001_MESHING_ENGINE_H

#include <vmath/vmath.h>
#include <functional>
#include <span>
#include <array>

#include "enums.h"
#include "gpu_buffer.h"
#include "ringbuffer.h"
#include "engine_context.h"
#include "chunk_id.h"

namespace ve001 {

struct MeshingEngine {
    /// @brief Descriptor of meshing, it maps to the ubo of binding id 2 in
    /// greedy meshing shader
    struct Descriptor {
        /// @brief offsets of submeshes (constant)
        alignas(16) vmath::u32 vbo_offsets[6][4];
        /// @brief position changes per meshing command (mutable)
        alignas(16) vmath::Vec3i32 chunk_position;
        /// @brief size of a chunk (constant)
        alignas(16) vmath::Vec3i32 chunk_size;
    };

    /// @brief holds temporary data of current meshing command execution
    struct Temp {
        /// @brief written vertex counts by the current meshing command execution
        /// (depending on <_chunk_size> there could be a need to invoke meshing
        /// few times per chunk). It is written and read in the shader.
        // vmath::u32 written_vertices_in_dwords[6] = {0};

        /// @brief written quads' counts by the current meshing command execution
        /// (depending on <_chunk_size> there could be a need to invoke meshing
        /// few times per chunk). It is written and read in the shader.
        vmath::u32 written_quads[6] = {0};
        /// @brief axes steps of the current meshing command execution
        /// (depending on <_chunk_size> there could be a need to invoke meshing
        /// few times per chunk). It is written and read in the shader.
        vmath::u32 axes_steps[6] = {0};
    };

    /// @brief command descripting meshing execution of a single chunk
    struct Command {
        /// @brief id of chunk meshed by command
        ChunkId chunk_id; 
        /// @brief position of the chunk
        vmath::Vec3i32 chunk_position;
        /// @brief pointer to voxel data to be issued before meshing starts
        std::span<const vmath::u16> voxel_data;
        /// @brief offset into vbo containing a mesh (maps to vbo in ChunkPool)
        vmath::u64 vbo_offset;
        /// @brief gl fence for which to wait in case the command is active one
        /// also indicates !!!IF COMMAND IS INITIALIZED (nullptr here if not)!!!
        void* fence{ nullptr };
        /// @brief axis progress keeps track of how many planes on each axes were meshed
        vmath::i32 axis_progress;
    };

    /// @brief is an interface and holds completed command data 
    struct Result {
        /// @brief indentifier of a succesfully meshed chunk (maps to
        /// chunk id in ChunkPool)
        ChunkId chunk_id;
        /// @brief written indices count *per face* indexed with ve001::Face enum.
        /// determines how many indices to render
        std::array<vmath::u32, 6> written_indices;
    };

    /// @brief id of buffer holding voxel data for subsequent meshing command execution
    /// (WRITE_ONLY, PERSISTENT, COHERENT)
    vmath::u32 _ssbo_voxel_data_id{ 0U };
    /// @brief pointer to mapped _ssbo_voxel_data_id (persistent)
    void* _ssbo_voxel_data_ptr{ nullptr };
    /// @brief gpu buffer for <MeshingDescriptor> data
    GPUBuffer _ubo_meshing_descriptor{ sizeof(Descriptor) };
    /// @brief gpu buffer for <MeshingTemp> data
    GPUBuffer _ssbo_meshing_temp{ sizeof(Temp) };
    /// @brief id of vbo holding meshes (the same vbo as in ChunkPool)
    vmath::u32 _vbo_id{ 0U };

    /// @brief buffer of pending meshing commands
    RingBuffer<Command> _commands;
    /// @brief meshing command currently in execution/waiting for poll
    Command _active_command;

    const EngineContext& _engine_context;

    MeshingEngine(const EngineContext& engine_context) 
        : _engine_context(engine_context), 
          _commands(512, Command{}) {}

    void init(vmath::u32 vbo_id);

    /// @brief issues meshing command to the engine
    /// @param chunk_id id of processed chunk. Maps to chunk ids in ChunkPool
    /// @param chunk_position chunk position
    /// @param voxel_data pointer to voxel_data based on which the meshing will take place
    /// @param vbo_offset offset into VBO from ChunkPool (needed for range binding VBO)
    /// @param vbo_size size of VBO from <vbo_offset> from ChunkPool (needed for range binding VBO)
    void issueMeshingCommand(
        ChunkId chunk_id,
        vmath::Vec3i32 chunk_position,
        std::span<const vmath::u16> voxel_data,
        vmath::u64 vbo_offset
    );

    /// @brief Function polls for the result from next command. It isn't waiting (the call
    // is non blocking), only checks once.
    /// @param result variable to which the function write result into
    /// @return true if valid value was written into the <future> param false if not
    bool pollMeshingCommand(Result& result);

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