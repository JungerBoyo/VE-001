#include "meshing_engine2.h"

#include "vertex.h"

#include <glad/glad.h>

#include <cstring>

using namespace ve001;
using namespace vmath;

#define VE001_SH_CONFIG_UBO_BINDING_MESHING_DESCRIPTOR 2
#define VE001_SH_CONFIG_SSBO_BINDING_VOXEL_DATA 5
#define VE001_SH_CONFIG_SSBO_BINDING_MESHING_TEMP 6
#define VE001_SH_CONFIG_SSBO_BINDING_MESH_DATA 7

void MeshingEngine::init(u32 vbo_id) {
    _vbo_id = vbo_id;

    glCreateBuffers(1, &_ssbo_voxel_data_id);
    glNamedBufferStorage(
        _ssbo_voxel_data_id,
        _engine_context.chunk_voxel_data_size,
        nullptr,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );
    _ssbo_voxel_data_ptr = glMapNamedBufferRange(
        _ssbo_voxel_data_id,
        0U,
        _engine_context.chunk_voxel_data_size,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    if (_ssbo_voxel_data_ptr == nullptr) {
        glDeleteBuffers(1, &_ssbo_voxel_data_id);
        return;// TODO: Error
    }

    _ubo_meshing_descriptor.init();
    _ubo_meshing_descriptor.bind(GL_UNIFORM_BUFFER, VE001_SH_CONFIG_UBO_BINDING_MESHING_DESCRIPTOR);
    _ssbo_meshing_temp.init();
    _ssbo_meshing_temp.bind(GL_SHADER_STORAGE_BUFFER, VE001_SH_CONFIG_SSBO_BINDING_MESHING_TEMP);

    Descriptor meshing_descriptor = {
        .vbo_offsets = { 
            _engine_context.chunk_max_submesh_size * 0, // +x
            _engine_context.chunk_max_submesh_size * 1, // -x
            _engine_context.chunk_max_submesh_size * 2, // +y
            _engine_context.chunk_max_submesh_size * 3, // -y
            _engine_context.chunk_max_submesh_size * 4, // +z
            _engine_context.chunk_max_submesh_size * 5 // +z
        },
        .chunk_position = {0, 0, 0},
        .chunk_size = _engine_context.chunk_size
    };
    _ubo_meshing_descriptor.write(static_cast<const void*>(&meshing_descriptor));

    Temp meshing_temp{};
    _ssbo_meshing_temp.write(static_cast<const void*>(&meshing_temp));
}

void MeshingEngine::issueMeshingCommand(u32 chunk_id, Vec3i32 chunk_position, std::span<u16> voxel_data, u32 vbo_offset, u32 vbo_size) {
    Command cmd = {
        .chunk_id       = chunk_id,
        .chunk_position = chunk_position,
        .voxel_data     = voxel_data,
        .vbo_offset     = vbo_offset,
        .vbo_size       = vbo_size,
        .fence          = nullptr,
        .axis_progress  = 0
    };

    if (_commands.empty()) {
        _active_command = cmd;
        firstCommandExec(_active_command);
        return;
    }

    if (!_commands.write(cmd)) {
        return; //TODO: Error
    }
}

bool MeshingEngine::pollMeshingCommand(Future& future) {
    if (_active_command.fence == nullptr) {
        return false;
    }

    const auto wait_result = glClientWaitSync(static_cast<GLsync>(_active_command.fence), 0, 0);
    if (wait_result == GL_WAIT_FAILED) {
        return false; //TODO: error
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

    glDeleteSync(static_cast<GLsync>(_active_command.fence));
    
    future.chunk_id = _active_command.chunk_id;
    _ssbo_meshing_temp.read(
        static_cast<void*>(future.written_vertices_counts.data()),
        offsetof(Temp, written_vertex_counts),
        sizeof(Temp::written_vertex_counts)
    );
    
    if (!_commands.read(_active_command)) {
        _active_command.fence = nullptr;
    } else {
        firstCommandExec(_active_command);
    }

    return true;
}

void MeshingEngine::firstCommandExec(Command& command) {
    std::memcpy(_ssbo_voxel_data_ptr, static_cast<const void*>(command.voxel_data.data()), _engine_context.chunk_voxel_data_size);

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
        command.vbo_offset, command.vbo_size
    );

    command.axis_progress += _engine_context.meshing_axis_progress_step;

    _engine_context.shader_repo[ShaderType::GREEDY_MESHING_SHADER].bind();
    glDispatchCompute(6, 1, 1);
    command.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void MeshingEngine::subsequentCommandExec(Command& command) {
    command.axis_progress += _engine_context.meshing_axis_progress_step;

    glDeleteSync(static_cast<GLsync>(command.fence));

    _engine_context.shader_repo[ShaderType::GREEDY_MESHING_SHADER].bind();
    glDispatchCompute(6, 1, 1);
    command.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void MeshingEngine::deinit() {
    if (_ssbo_voxel_data_ptr != nullptr) {
        glUnmapNamedBuffer(_ssbo_voxel_data_id);
    }
    _ssbo_voxel_data_ptr = nullptr;
    glDeleteBuffers(1, &_ssbo_voxel_data_id);
    _ubo_meshing_descriptor.deinit();
    _ssbo_meshing_temp.deinit();
}