#include "meshing_engine_cpu.h"

#include <glad/glad.h>

#include <cstring>


using namespace ve001;
using namespace vmath;

MeshingEngineCPU::MeshingEngineCPU(const EngineContext& engine_context, vmath::u32 max_chunks, vmath::u32 threads_count) noexcept : MeshingEngineBase(engine_context), _cpu_mesher(engine_context, threads_count, max_chunks) {
	while(!_cpu_mesher.ready()) {}

    try {
        _commands.resize(max_chunks);
    } catch(const std::exception&) {
        _engine_context.error |= Error::CPU_ALLOCATION_FAILED;
    }
}

void MeshingEngineCPU::init(vmath::u32 vbo_id) noexcept {
	_vbo_id = vbo_id;

    glCreateBuffers(1, &_staging_buffer_id);
    glNamedBufferStorage(
        _staging_buffer_id,
        _engine_context.chunk_max_current_mesh_size,
        nullptr,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    if (glGetError() == GL_OUT_OF_MEMORY) {
        _engine_context.error |= Error::GPU_ALLOCATION_FAILED;
        return;
    }

    _staging_buffer_ptr = glMapNamedBufferRange(
        _staging_buffer_id,
        0U,
        _engine_context.chunk_max_current_mesh_size,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    if (_staging_buffer_ptr == nullptr) {
        glDeleteBuffers(1, &_staging_buffer_id);
        _engine_context.error |= Error::GPU_BUFFER_MAPPING_FAILED;
        return;
    }
}

void MeshingEngineCPU::issueMeshingCommand(ChunkId chunk_id, vmath::Vec3f32 chunk_position, std::span<const vmath::u16> voxel_data) noexcept {
	_commands.write(chunk_id, _cpu_mesher.mesh(chunk_position, voxel_data));
}

bool MeshingEngineCPU::pollMeshingCommand(Result& result) noexcept {
	CommandCPU* cmd;
	if (!_commands.peek(cmd))
		return false;
	
	if (cmd->future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return false;

	_commands.emptyRead();

	auto value = cmd->future.get();

	result.chunk_id = cmd->chunk_id;
    result.written_indices[X_POS] = value.written_quads[X_POS] * 6U;
    result.written_indices[X_NEG] = value.written_quads[X_NEG] * 6U;
    result.written_indices[Y_POS] = value.written_quads[Y_POS] * 6U;
    result.written_indices[Y_NEG] = value.written_quads[Y_NEG] * 6U;
    result.written_indices[Z_POS] = value.written_quads[Z_POS] * 6U;
    result.written_indices[Z_NEG] = value.written_quads[Z_NEG] * 6U;
	result.overflow_flag = value.overflow_flag;

	if (!result.overflow_flag) {
		if (_fence != nullptr) {
			const auto wait_result = glClientWaitSync(static_cast<GLsync>(_fence), 0, UINT64_MAX);
			if (wait_result == GL_WAIT_FAILED) {
				_engine_context.error |= Error::FENCE_WAIT_FAILED;
				_fence = nullptr;
				return false;
			}
			if (wait_result == GL_TIMEOUT_EXPIRED) {
				_fence = nullptr;
				return false;
			}
    		glDeleteSync(static_cast<GLsync>(_fence));
		}

		memcpy(_staging_buffer_ptr, static_cast<const void*>(value.staging_buffer_ptr.data()), 
				_engine_context.chunk_max_current_mesh_size);
		value.staging_buffer_in_use_flag->store(false, std::memory_order_release);
		glCopyNamedBufferSubData(_staging_buffer_id, _vbo_id, 0, 
			static_cast<GLintptr>(static_cast<u64>(result.chunk_id) * 
				_engine_context.chunk_max_current_mesh_size),
			static_cast<GLintptr>(_engine_context.chunk_max_current_mesh_size)
		);
		_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	}
#ifdef ENGINE_TEST
	value.cmd_timer_real.stop();
	result_meshing_time_ns = value.cmd_timer_meshing.duration;
	result_real_meshing_time_ns = value.cmd_timer_real.duration;
#endif
	return true;
}

void MeshingEngineCPU::updateMetadata(vmath::u32 new_vbo_id) noexcept {
	deinit();
	init(new_vbo_id);
	_cpu_mesher.updateLimits();
}

void MeshingEngineCPU::deinit() noexcept {
	glUnmapNamedBuffer(_staging_buffer_id);
	_staging_buffer_ptr = nullptr;
	glDeleteBuffers(1, &_staging_buffer_id);
	_staging_buffer_id = 0;
}
