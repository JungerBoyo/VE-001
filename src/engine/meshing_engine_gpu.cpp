#include "meshing_engine_gpu.h"

#include "vertex.h"

#include <glad/glad.h>

#include <cstring>

#include <iostream>

using namespace ve001;
using namespace vmath;

MeshingEngineGPU::MeshingEngineGPU(const EngineContext& engine_context, vmath::u32 max_chunks) noexcept 
    : MeshingEngineBase(engine_context) {
    try {
        _commands.resize(max_chunks);
    } catch(const std::exception&) {
        _engine_context.error |= Error::CPU_ALLOCATION_FAILED;
    }
}


static bool deviceSupportsARBSpirv() noexcept {
	int ext_num{ 0 };
    glGetIntegerv(GL_NUM_EXTENSIONS, &ext_num);
    for (int i{ 0 }; i < ext_num; ++i) {
        const auto ext_str = std::string_view(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)));
        if (ext_str == "GL_ARB_gl_spirv") {
			return true;
        }
    }
	return false;
}

void MeshingEngineGPU::init(u32 vbo_id) noexcept {
    _vbo_id = vbo_id;

#ifdef USE_VOLUME_TEXTURE_3D
	glCreateTextures(GL_TEXTURE_3D, 1, &_volume_3d_texture_id);
	glTextureStorage3D(
			_volume_3d_texture_id, 1, GL_R16UI,
			_engine_context.chunk_size[0],
			_engine_context.chunk_size[1],
			_engine_context.chunk_size[2]);
#else
    glCreateBuffers(1, &_ssbo_voxel_data_id);
    glNamedBufferStorage(
        _ssbo_voxel_data_id,
        _engine_context.chunk_voxel_data_size,
        nullptr,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    if (glGetError() == GL_OUT_OF_MEMORY) {
        _engine_context.error |= Error::GPU_ALLOCATION_FAILED;
        return;
    }
	
    _ssbo_voxel_data_ptr = glMapNamedBufferRange(
        _ssbo_voxel_data_id,
        0U,
        _engine_context.chunk_voxel_data_size,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    if (_ssbo_voxel_data_ptr == nullptr) {
        glDeleteBuffers(1, &_ssbo_voxel_data_id);
        _engine_context.error |= Error::GPU_BUFFER_MAPPING_FAILED;
        return;
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VE001_SH_CONFIG_SSBO_BINDING_VOXEL_DATA, _ssbo_voxel_data_id);
#endif

    _engine_context.error |= _ubo_meshing_descriptor.init();
    _ubo_meshing_descriptor.bind(GL_UNIFORM_BUFFER, VE001_SH_CONFIG_UBO_BINDING_MESHING_DESCRIPTOR);
    _engine_context.error |= _ssbo_meshing_temp.init();
    _ssbo_meshing_temp.bind(GL_SHADER_STORAGE_BUFFER, VE001_SH_CONFIG_SSBO_BINDING_MESHING_TEMP);

    Descriptor meshing_descriptor = {
        .vbo_offsets = {  // passed in floats
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 0UL), 0U, 0U, 0U }, // +x
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 1UL), 0U, 0U, 0U }, // -x
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 2UL), 0U, 0U, 0U }, // +y
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 3UL), 0U, 0U, 0U }, // -y
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 4UL), 0U, 0U, 0U }, // +z
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 5UL), 0U, 0U, 0U }  // -z
        },
        .max_submesh_size_in_quads = static_cast<u32>(_engine_context.chunk_max_current_submesh_size/(sizeof(Vertex) * 4UL)),
        .chunk_position = {0.F, 0.F, 0.F},
        .chunk_size = _engine_context.chunk_size
    };
    _ubo_meshing_descriptor.write(static_cast<const void*>(&meshing_descriptor));

    Temp meshing_temp{};
    _ssbo_meshing_temp.write(static_cast<const void*>(&meshing_temp));

    _meshing_shader.init();
#ifdef FORCE_USE_SHADER_FROM_SRC
	if (!_meshing_shader.attach(_engine_context.meshing_shader_src_path, false)) {
		_engine_context.error |= Error::SHADER_ATTACH_FAILED;
		return;
	}
#else
    if (_engine_context.meshing_shader_bin_path.has_value() && deviceSupportsARBSpirv()) {
        if (!_meshing_shader.attach(_engine_context.meshing_shader_bin_path.value(), true)) {
            _engine_context.error |= Error::SHADER_ATTACH_FAILED;
            return;
        }
    } else {
        if (!_meshing_shader.attach(_engine_context.meshing_shader_src_path, false)) {
            _engine_context.error |= Error::SHADER_ATTACH_FAILED;
            return;
        }
    }
#endif
#ifdef ENGINE_TEST
    u32 tmp[2] = { 0U, 0U };
    glCreateQueries(GL_TIMESTAMP, 2, tmp);
    gpu_meshing_time_query = tmp[0];
    real_meshing_time_query = tmp[1];
#endif
}

void MeshingEngineGPU::issueMeshingCommand(ChunkId chunk_id, vmath::Vec3f32 chunk_position, std::span<const vmath::u16> voxel_data) noexcept {
    Command cmd = {
        .chunk_id       = chunk_id,
        .chunk_position = chunk_position,
        .voxel_data     = voxel_data,
        .fence          = nullptr,
        .axis_progress  = 0
    };

    if (_active_command.fence == nullptr) {
        _active_command = cmd;
        firstCommandExec(_active_command);
        return;
    }

    /// will never return false since size == max chunks count
    _commands.write(std::move(cmd));
}

bool MeshingEngineGPU::pollMeshingCommand(Result& result) noexcept {
    if (_active_command.fence == nullptr) {
        return false;
    }

    const auto wait_result = glClientWaitSync(static_cast<GLsync>(_active_command.fence), 0, 0);
    if (wait_result == GL_WAIT_FAILED) {
        _engine_context.error |= Error::FENCE_WAIT_FAILED;
        return false;
    }
    if (wait_result == GL_TIMEOUT_EXPIRED) {
        return false;
    }

    // otherwise GL_ALREADY_SIGNALED or GL_CONDITION_SATISFIED
    if (_active_command.axis_progress < _engine_context.chunk_size[0] ||
        _active_command.axis_progress < _engine_context.chunk_size[1] ||
        _active_command.axis_progress < _engine_context.chunk_size[2]
    ) {
        subsequentCommandExec(_active_command);
        return false;
    }

#ifdef ENGINE_TEST
    glGetQueryObjectui64v(gpu_meshing_time_query, GL_QUERY_RESULT, &end_meshing_time_ns);
    result_gpu_meshing_time_ns = end_meshing_time_ns - begin_meshing_time_ns;
    glQueryCounter(real_meshing_time_query, GL_TIMESTAMP);
    glGetQueryObjectui64v(real_meshing_time_query, GL_QUERY_RESULT, &end_meshing_time_ns);
    result_real_meshing_time_ns = end_meshing_time_ns - begin_meshing_time_ns;
#endif

    glDeleteSync(static_cast<GLsync>(_active_command.fence));
    _active_command.fence = nullptr;
    
    result.chunk_id = _active_command.chunk_id;

    Descriptor desc{};
    _ubo_meshing_descriptor.read(static_cast<void*>(&desc), 0, sizeof(Descriptor));

    Temp temp{};
    _ssbo_meshing_temp.read(static_cast<void*>(&temp), 0, sizeof(Temp));
    result.written_indices[X_POS] = temp.written_quads[X_POS] * 6U;
    result.written_indices[X_NEG] = temp.written_quads[X_NEG] * 6U;
    result.written_indices[Y_POS] = temp.written_quads[Y_POS] * 6U;
    result.written_indices[Y_NEG] = temp.written_quads[Y_NEG] * 6U;
    result.written_indices[Z_POS] = temp.written_quads[Z_POS] * 6U;
    result.written_indices[Z_NEG] = temp.written_quads[Z_NEG] * 6U;

    result.overflow_flag = static_cast<bool>(temp.overflow_flag);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VE001_SH_CONFIG_SSBO_BINDING_MESH_DATA, 0);

    if (!result.overflow_flag) {
        if (_commands.read(_active_command)) {
            firstCommandExec(_active_command);
        }
    }
    return true;
}

void MeshingEngineGPU::updateMetadata(vmath::u32 new_vbo_id) noexcept {
    _vbo_id = new_vbo_id;

    Descriptor meshing_descriptor = {
        .vbo_offsets = {  // passed in floats
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 0UL), 0U, 0U, 0U }, // +x
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 1UL), 0U, 0U, 0U }, // -x
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 2UL), 0U, 0U, 0U }, // +y
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 3UL), 0U, 0U, 0U }, // -y
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 4UL), 0U, 0U, 0U }, // +z
            {static_cast<u32>(_engine_context.chunk_max_current_submesh_size/sizeof(f32) * 5UL), 0U, 0U, 0U }  // -z
        },
        .max_submesh_size_in_quads = static_cast<u32>(_engine_context.chunk_max_current_submesh_size/(sizeof(Vertex) * 4UL)),
        .chunk_position = {0.F, 0.F, 0.F},
        .chunk_size = _engine_context.chunk_size
    };
    _ubo_meshing_descriptor.write(static_cast<const void*>(&meshing_descriptor));

    Temp meshing_temp{};
    _ssbo_meshing_temp.write(static_cast<const void*>(&meshing_temp));
}

void MeshingEngineGPU::firstCommandExec(Command& command) noexcept {
#ifdef ENGINE_TEST
    glQueryCounter(gpu_meshing_time_query, GL_TIMESTAMP);
    glGetQueryObjectui64v(gpu_meshing_time_query, GL_QUERY_RESULT, &begin_meshing_time_ns);
#endif

#ifdef USE_VOLUME_TEXTURE_3D
	glTextureSubImage3D(_volume_3d_texture_id, 0, 0, 0, 0, 
			_engine_context.chunk_size[0],
			_engine_context.chunk_size[1],
			_engine_context.chunk_size[2],
			GL_RED_INTEGER, GL_UNSIGNED_SHORT,
			static_cast<const void*>(command.voxel_data.data()));
	glBindImageTexture(VE001_SH_CONFIG_IMAGE_BINDING_VOLUME_3D, _volume_3d_texture_id, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R16UI);
#else
    std::memcpy(_ssbo_voxel_data_ptr, static_cast<const void*>(command.voxel_data.data()), _engine_context.chunk_voxel_data_size);
#endif

    Temp meshing_temp{};
    _ssbo_meshing_temp.write(static_cast<const void*>(&meshing_temp));

    _ubo_meshing_descriptor.write(
        static_cast<const void*>(&command.chunk_position),
        offsetof(Descriptor, chunk_position),
        sizeof(Descriptor::chunk_position)
    );

    glBindBufferRange(
        GL_SHADER_STORAGE_BUFFER, 
        VE001_SH_CONFIG_SSBO_BINDING_MESH_DATA, _vbo_id,
        static_cast<GLintptr>(static_cast<u64>(command.chunk_id) * _engine_context.chunk_max_current_mesh_size), 
        static_cast<GLintptr>(_engine_context.chunk_max_current_mesh_size)
    );

    command.axis_progress += _engine_context.meshing_axis_progress_step;

    _meshing_shader.bind();

#ifdef ENGINE_MEMORY_TEST
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_UNIFORM_BARRIER_BIT);
	glFlushMappedNamedBufferRange(_ssbo_voxel_data_id, 0, _engine_context.chunk_voxel_data_size);
	glQueryCounter(gpu_meshing_time_query, GL_TIMESTAMP);
    glGetQueryObjectui64v(gpu_meshing_time_query, GL_QUERY_RESULT, &end_meshing_time_ns);
	result_gpu_meshing_setup_time_ns = end_meshing_time_ns - begin_meshing_time_ns;
#endif
    glDispatchCompute(6, 1, 1);
#ifdef ENGINE_TEST
    glQueryCounter(gpu_meshing_time_query, GL_TIMESTAMP);
#endif
    command.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void MeshingEngineGPU::subsequentCommandExec(Command& command) noexcept {
    command.axis_progress += _engine_context.meshing_axis_progress_step;

    glDeleteSync(static_cast<GLsync>(command.fence));

    _meshing_shader.bind();
    glDispatchCompute(6, 1, 1);
    command.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void MeshingEngineGPU::deinit() noexcept {
#ifdef ENGINE_TEST
    u32 tmp[2] = {gpu_meshing_time_query, real_meshing_time_query};
    glDeleteQueries(2, tmp);
#endif
    _meshing_shader.deinit();
#ifdef USE_VOLUME_TEXTURE_3D
	glDeleteTextures(1, &_volume_3d_texture_id);
#else
    if (_ssbo_voxel_data_ptr != nullptr) {
        glUnmapNamedBuffer(_ssbo_voxel_data_id);
    }
    _ssbo_voxel_data_ptr = nullptr;
    glDeleteBuffers(1, &_ssbo_voxel_data_id);
#endif
    _ubo_meshing_descriptor.deinit();
    _ssbo_meshing_temp.deinit();
}
