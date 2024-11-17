#ifndef VE001_MESHING_ENGINE_GPU_H
#define VE001_MESHING_ENGINE_GPU_H

#include <vmath/vmath.h>

#include "gpu_buffer.h"
#include "engine_context.h"
#include "shader.h"
#include "meshing_engine_base.h"

namespace ve001 {

struct MeshingEngineGPU : public MeshingEngineBase {
	using Result = MeshingEngineBase::Result;

    /// @brief command descripting meshing execution of a single chunk
    struct Command {
        /// @brief id of chunk meshed by command
        ChunkId chunk_id; 
        /// @brief position of the chunk
        vmath::Vec3f32 chunk_position;
        /// @brief pointer to voxel data to be issued before meshing starts
        std::span<const vmath::u16> voxel_data;
        /// @brief gl fence for which to wait in case the command is active one
        /// also indicates !!!IF COMMAND IS INITIALIZED (nullptr here if not)!!!
        void* fence{ nullptr };
        /// @brief axis progress keeps track of how many planes on each axes were meshed
        vmath::i32 axis_progress;
    };

    /// @brief Descriptor of meshing, it maps to the ubo of binding id 2 in
    /// greedy meshing shader
    struct Descriptor {
        /// @brief offsets of submeshes (constant)
        alignas(16) vmath::u32 vbo_offsets[6][4];
        /// @brief max submesh size in quads 
        alignas(16) vmath::u32 max_submesh_size_in_quads;
        /// @brief position changes per meshing command (mutable)
        alignas(16) vmath::Vec3f32 chunk_position;
        /// @brief size of a chunk (constant)
        alignas(16) vmath::Vec3i32 chunk_size;
    };

    /// @brief holds temporary data of current meshing command execution
    struct Temp {
        /// @brief written quads' counts by the current meshing command execution
        /// (depending on <_chunk_size> there could be a need to invoke meshing
        /// few times per chunk). It is written and read in the shader.
        vmath::u32 written_quads[6] = {0};
        /// @brief axes steps of the current meshing command execution
        /// (depending on <_chunk_size> there could be a need to invoke meshing
        /// few times per chunk). It is written and read in the shader.
        vmath::u32 axes_steps[6] = {0};
        /// @brief set to 1 if any of vbo submesh regions overflowed in the last 
        /// execution
        vmath::u32 overflow_flag{ 0U };
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

    /// @brief greedy meshing shader handle
    Shader _meshing_shader;

#ifdef ENGINE_TEST
    vmath::u64 begin_meshing_time_ns{ 0UL };
    vmath::u64 end_meshing_time_ns{ 0UL };
    vmath::u64 result_gpu_meshing_time_ns{ 0UL };
    vmath::u64 result_gpu_meshing_setup_time_ns{ 0UL };
    vmath::u64 result_real_meshing_time_ns{ 0UL };
    vmath::u32 gpu_meshing_time_query{ 0U };
    vmath::u32 real_meshing_time_query{ 0U };
#endif

    MeshingEngineGPU(const EngineContext& engine_context, vmath::u32 max_chunks) noexcept;

    void init(vmath::u32 vbo_id) noexcept override;

    /// @brief issues meshing command to the engine
    /// @param chunk_id id of processed chunk. Maps to chunk ids in ChunkPool
    /// @param chunk_position chunk position
    /// @param voxel_data pointer to voxel_data based on which the meshing will take place
    void issueMeshingCommand(ChunkId chunk_id, vmath::Vec3f32 chunk_position, std::span<const vmath::u16> voxel_data) noexcept override;

    /// @brief Function polls for the result from next command. It isn't waiting (the call
    // is non blocking), only checks once.
    /// @param result variable to which the function write result into
    /// @return true if valid value was written into the <future> param false if not
    bool pollMeshingCommand(Result& result) noexcept override;

    /// @brief updates metadata based on engine context and new vbo id
    /// @param new_vbo_id new vbo id to which to write meshes
    void updateMetadata(vmath::u32 new_vbo_id) noexcept override;

    /// @brief executes command meaning dispatches meshing based on parameters 
    /// in the <command>. It is first execution so the data is passed to the gpu here
    /// @param command command to be executed
    void firstCommandExec(Command& command) noexcept;

    /// @brief executes command meaning dispatches meshing based on parameters 
    /// in the <command>. It is subsequent command execution so the per command data
    /// isn't allocated.
    /// @param command command to be executed
    void subsequentCommandExec(Command& command) noexcept;

#ifdef ENGINE_TEST		
	std::tuple<vmath::u64, vmath::u64, vmath::u64>
		getBenchmarkData() const noexcept override {
		return {result_gpu_meshing_time_ns, result_real_meshing_time_ns,
			result_gpu_meshing_setup_time_ns};
	}
#endif
    void deinit() noexcept override;
};

}

#endif
