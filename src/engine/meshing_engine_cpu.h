#ifndef VE001_MESHING_ENGINE_CPU_H
#define VE001_MESHING_ENGINE_CPU_H

#include <thread>

#include <vmath/vmath.h>

#include "gpu_buffer.h"
#include "engine_context.h"
#include "shader.h"
#include "cpu_mesher.h"
#include "meshing_engine_base.h"

#ifdef ENGINE_TEST
#include <timer.h>
#endif

namespace ve001 {

struct MeshingEngineCPU : public MeshingEngineBase {
	using Result = MeshingEngineBase::Result;

	struct CommandCPU {
		ChunkId chunk_id;
		std::future<CpuMesher::Promise> future;
	};

	void* _fence{ nullptr };

	void* _staging_buffer_ptr{ nullptr };
	vmath::u32 _staging_buffer_id{ 0U };

    /// @brief id of buffer holding voxel data for subsequent meshing command execution
    vmath::u32 _vbo_id{ 0U };
	/// @brief cpu mesher - performs the meshing work
	CpuMesher _cpu_mesher;
    /// @brief buffer of pending meshing commands
    RingBuffer<CommandCPU> _commands;
#ifdef ENGINE_TEST
    vmath::u64 result_meshing_time_ns{ 0UL };
    vmath::u64 result_real_meshing_time_ns{ 0UL };
#endif

    MeshingEngineCPU(const EngineContext& engine_context, vmath::u32 max_chunks,
			vmath::u32 threads_count) noexcept;

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

#ifdef ENGINE_TEST		
	std::tuple<vmath::u64, vmath::u64, vmath::u64>
		getBenchmarkData() const noexcept override {
		return {result_meshing_time_ns, result_real_meshing_time_ns, 0};
	}
#endif

#ifdef ENGINE_TEST_NONINTERACTIVE
	virtual bool idle() const noexcept override {
		return _commands.empty();
	}
#endif
    void deinit() noexcept override;
};

}

#endif
