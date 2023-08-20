#include "chunk_mesh_pool.h"

#include <cstddef>
#include <cstring>

#include <glad/glad.h>

using namespace ve001;
using namespace vmath;

Error ChunkMeshPool::init(Config config) noexcept {
    _max_chunk_size = 
        config.chunk_dimensions[0] *
        config.chunk_dimensions[1] *
        config.chunk_dimensions[2] * 
        config.vertex_size * (36/2); // worst case, 3D checkerboard
    _max_submesh_size = _max_chunk_size / 6;

    u32 tmp[2] = { 0U, 0U };

    glCreateBuffers(2, tmp);
    _vbo_id = tmp[0];
    _dibo_id = tmp[1];

    glNamedBufferStorage(
        _vbo_id,
        _max_chunk_size * config.max_chunks,
        nullptr,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    glNamedBufferStorage(
        _dibo_id,
        6 * config.max_chunks * sizeof(DrawArraysIndirectCmd),
        nullptr,
        GL_DYNAMIC_STORAGE_BIT
    );

    glCreateVertexArrays(1, &_vao_id);
    config.vertex_layout_config(_vao_id, _vbo_id);

    _vbo_ptr = glMapNamedBuffer(
        _vbo_id,
        GL_WRITE_ONLY
        // GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    if (_vbo_ptr == nullptr) {
        glDeleteBuffers(2, tmp);
        glDeleteVertexArrays(1, &_vao_id);
        return Error::FAILED_TO_MAP_VBO_BUFFER;
    }

    _chunks_count = config.max_chunks;
    _submeshes_count = 6 * config.max_chunks;
    _vertex_size = config.vertex_size;

    try {
        _free_chunks = RingBuffer<FreeChunk>(_chunks_count, {});

        for (std::size_t c{ 0U }; c < _chunks_count; ++c) {
            _free_chunks.pushBack({
                .chunk_id = static_cast<u32>(c),
                .region = static_cast<void*>(static_cast<u8*>(_vbo_ptr) + c * _max_chunk_size)
            });
        }
    } catch ([[maybe_unused]] const std::exception& e) {
        glDeleteBuffers(2, tmp);
        glDeleteVertexArrays(1, &_vao_id);
        _free_chunks.clear();
        return Error::CPU_MEMORY_ALLOCATION_FAILED;
    }

    return Error::NO_ERROR;
}

Error ChunkMeshPool::allocateChunk(u32& chunk_id, ChunkData chunk_data, Vec3f32 position) noexcept {
    chunk_id = INT32_MAX; 
    
    if (_vbo_ptr == nullptr) {
        return Error::USED_UNINITIALIZED;
    }

    if (chunk_data.size == 0U || chunk_data.size > _max_chunk_size) {
        return Error::INVALID_CHUNK_SIZE;
    }

    if (_free_chunks.empty()) {
        return Error::NO_FREE_CHUNKS;
    }

    FreeChunk free_chunk{};
    _free_chunks.popBack(free_chunk);

    const auto base_cmd_index = static_cast<u32>(_draw_cmds.size());
    _chunk_metadata.emplace_back(ChunkMetadata{
        .position = position, 
        .draw_cmd_indices = {
            base_cmd_index + X_POS, base_cmd_index + X_NEG,
            base_cmd_index + Y_POS, base_cmd_index + Y_NEG,
            base_cmd_index + Z_POS, base_cmd_index + Z_NEG,
        },
        .region = free_chunk.region,
        .chunk_id = free_chunk.chunk_id 
    });

    Error result{ Error::NO_ERROR };

    std::size_t i{ 0U };
    for (;i < 6U; ++i) {
        auto& submesh = chunk_data.submesh_data[i];
        if (submesh.vertex_count == 0U || submesh.vertex_count * _vertex_size > _max_submesh_size) {
            result = Error::INVALID_SUBMESH_SIZE;
            break;
        }
        if (submesh.ptr == nullptr) {
            result = Error::INVALID_SUBMESH_PTR;
            break;
        }

        void* submesh_offset = static_cast<void*>(static_cast<u8*>(free_chunk.region) + i * _max_submesh_size);

        _draw_cmds.emplace_back(
            submesh.vertex_count, 
            1U, 
            (static_cast<u8*>(submesh_offset) - static_cast<u8*>(_vbo_ptr)) / _vertex_size, 
            0U, 
            submesh.orientation,
            free_chunk.chunk_id
        );

        std::memcpy(submesh_offset, submesh.ptr, submesh.vertex_count * _vertex_size);

    }

    if (result == Error::NO_ERROR) {
        chunk_id = free_chunk.chunk_id;
        _chunk_id_to_index.push_back(chunk_id);
    } else {
        _free_chunks.pushBack(free_chunk);
        _chunk_metadata.pop_back();

        for (std::size_t _i{ 0U }; _i < i; ++_i) {
            _draw_cmds.pop_back();
        }
    }

    return result;
}

Error ChunkMeshPool::allocateEmptyChunk(u32& chunk_id, Vec3f32 position) noexcept {
    chunk_id = INT32_MAX; 
    
    if (_vbo_ptr == nullptr) {
        return Error::USED_UNINITIALIZED;
    }

    if (_free_chunks.empty()) {
        return Error::NO_FREE_CHUNKS;
    }

    FreeChunk free_chunk{};
    _free_chunks.popBack(free_chunk);

    const auto base_cmd_index = static_cast<u32>(_draw_cmds.size());
    _chunk_metadata.emplace_back(ChunkMetadata{
        .position = position, 
        .draw_cmd_indices = {
            base_cmd_index + X_POS, base_cmd_index + X_NEG,
            base_cmd_index + Y_POS, base_cmd_index + Y_NEG,
            base_cmd_index + Z_POS, base_cmd_index + Z_NEG,
        },
        .region = free_chunk.region,
        .chunk_id = free_chunk.chunk_id
    });

    Error result{ Error::NO_ERROR };
    
    for (std::size_t i{ 0U }; i < 6U; ++i) {
        void* submesh_offset = static_cast<void*>(static_cast<u8*>(free_chunk.region) + i * _max_submesh_size);

        _draw_cmds.emplace_back(
            0U, 
            1U, 
            (static_cast<u8*>(submesh_offset) - static_cast<u8*>(_vbo_ptr)) / _vertex_size, 
            0U, 
            static_cast<SubmeshOrientation>(i),
            free_chunk.chunk_id
        );
    }

    chunk_id = free_chunk.chunk_id;
    _chunk_id_to_index.push_back(chunk_id);

    return result;
}

Error ChunkMeshPool::writeChunk(u32 chunk_id, ChunkData chunk_data) noexcept {
    if (chunk_id >= _chunk_id_to_index.size() || _chunk_id_to_index[chunk_id] == INVALID_CHUNK_INDEX) {
       return Error::CHUNK_DOES_NOT_EXIST; 
    }
    
    const auto chunk_index = _chunk_id_to_index[chunk_id];
    const auto chunk_metadata = _chunk_metadata[chunk_index];

    if (chunk_metadata.aquired) {
        return Error::CHUNK_POINTER_AQUIRED;
    }

    Error result{ Error::NO_ERROR };

    std::size_t i{ 0U };
    for (; i < 6U; ++i) {
        const auto submesh = chunk_data.submesh_data[i];
        if (submesh.vertex_count == 0U || submesh.vertex_count * _vertex_size > _max_submesh_size) {
            result = Error::INVALID_SUBMESH_SIZE;
            break;
        }
        if (submesh.ptr == nullptr) {
            result = Error::INVALID_SUBMESH_PTR;
            break;
        }

        const auto submesh_index = static_cast<std::size_t>(submesh.orientation);
            
        auto& draw_cmd = _draw_cmds[chunk_metadata.draw_cmd_indices[submesh_index]];

        draw_cmd.count = submesh.vertex_count;
        draw_cmd.orientation = submesh.orientation;

        void* submesh_offset = static_cast<void*>(static_cast<u8*>(chunk_metadata.region) + i * _max_submesh_size);

        std::memcpy(submesh_offset, submesh.ptr, submesh.vertex_count * _vertex_size);
    }

    if (result != Error::NO_ERROR) {
        for (std::size_t _i{ 0U }; _i < i; ++_i) {
            const auto submesh = chunk_data.submesh_data[i];
            const auto submesh_index = static_cast<std::size_t>(submesh.orientation);
                
            auto& draw_cmd = _draw_cmds[chunk_metadata.draw_cmd_indices[submesh_index]];
            draw_cmd.count = 0U;
        }
    }

    return result;
}

Error ChunkMeshPool::updateChunkSubmeshVertexCounts(u32 chunk_id, std::array<u32, 6> counts) noexcept {
    if (chunk_id >= _chunk_id_to_index.size() || _chunk_id_to_index[chunk_id] == INVALID_CHUNK_INDEX) {
       return Error::CHUNK_DOES_NOT_EXIST; 
    }
    
    const auto chunk_index = _chunk_id_to_index[chunk_id];
    const auto chunk_metadata = _chunk_metadata[chunk_index];

    if (chunk_metadata.aquired) {
        return Error::CHUNK_POINTER_AQUIRED;
    }

    Error result{ Error::NO_ERROR };

    std::size_t i{ 0U };
    for (; i < 6U; ++i) {
        if (counts[i] == 0U || counts[i] * _vertex_size > _max_submesh_size) {
            result = Error::INVALID_SUBMESH_SIZE;
            break;
        }
        auto& draw_cmd = _draw_cmds[chunk_metadata.draw_cmd_indices[i]];

        draw_cmd.count = counts[i];
        draw_cmd.orientation = static_cast<SubmeshOrientation>(i);
    }

    if (result != Error::NO_ERROR) {
        for (std::size_t _i{ 0U }; _i < i; ++_i) {
            auto& draw_cmd = _draw_cmds[chunk_metadata.draw_cmd_indices[i]];
            draw_cmd.count = 0U;
        }
    }

    return result;
}

Error ChunkMeshPool::aquireChunkWritePtr(u32 chunk_id, void*& dst) noexcept {
    if (chunk_id >= _chunk_id_to_index.size() || _chunk_id_to_index[chunk_id] == INVALID_CHUNK_INDEX) {
       return Error::CHUNK_DOES_NOT_EXIST; 
    }
    
    const auto chunk_index = _chunk_id_to_index[chunk_id];
    auto& chunk_metadata = _chunk_metadata[chunk_index];

    if (chunk_metadata.aquired) {
        return Error::CHUNK_POINTER_AQUIRED;
    }
    
    chunk_metadata.aquired = true;

    dst = chunk_metadata.region;

    return Error::NO_ERROR;
}

Error ChunkMeshPool::freeChunkWritePtr(u32 chunk_id) noexcept {
    if (chunk_id >= _chunk_id_to_index.size() || _chunk_id_to_index[chunk_id] == INVALID_CHUNK_INDEX) {
       return Error::CHUNK_DOES_NOT_EXIST; 
    }
    
    const auto chunk_index = _chunk_id_to_index[chunk_id];
    auto& chunk_metadata = _chunk_metadata[chunk_index];

    chunk_metadata.aquired = false;

    return Error::NO_ERROR;
}

Error ChunkMeshPool::deallocateChunk(u32 chunk_id) noexcept {
    if (chunk_id >= _chunk_id_to_index.size() || _chunk_id_to_index[chunk_id] == INVALID_CHUNK_INDEX) {
       return Error::CHUNK_DOES_NOT_EXIST; 
    }

    const auto chunk_index = _chunk_id_to_index[chunk_id];
    const auto chunk_metadata = _chunk_metadata[chunk_index];
    if (chunk_metadata.aquired) {
        return Error::CHUNK_POINTER_AQUIRED;
    }

    deallocateChunkDrawCmds(chunk_metadata);

    if (chunk_index != _chunk_metadata.size() - 1U) {
        const auto last_chunk_metadata = _chunk_metadata.back();

        _chunk_metadata[chunk_index] = last_chunk_metadata;
        _chunk_id_to_index[last_chunk_metadata.chunk_id] = chunk_index;
    }

    _chunk_id_to_index[chunk_id] = INVALID_CHUNK_INDEX;
    _free_chunks.pushBack({
        .chunk_id = chunk_id,
        .region = chunk_metadata.region
    });
    _chunk_metadata.pop_back();

    return Error::NO_ERROR;
}

void ChunkMeshPool::deallocateChunkDrawCmds(const ChunkMetadata& chunk_metadata) {
    for (std::size_t i{ 0U }; i < 6U; ++i) {
        const auto draw_cmd_index = chunk_metadata.draw_cmd_indices[i];
        if (draw_cmd_index != _draw_cmds.size() - 1U) {
            auto& last_draw_cmd = _draw_cmds.back();
            const auto last_draw_cmd_chunk_index = _chunk_id_to_index[last_draw_cmd.chunk_id];
            auto& last_draw_cmd_chunk = _chunk_metadata[last_draw_cmd_chunk_index];

            const auto last_draw_cmd_submesh_index = static_cast<u32>(last_draw_cmd.orientation);
            last_draw_cmd_chunk.draw_cmd_indices[last_draw_cmd_submesh_index] = draw_cmd_index;

            _draw_cmds[draw_cmd_index] = last_draw_cmd;
        }
        _draw_cmds.pop_back();
    }
}


void ChunkMeshPool::drawAll() noexcept {
    if (_draw_cmds.size() > 0U) {
        glNamedBufferSubData(
            _dibo_id, 0, 
            _draw_cmds.size() * sizeof(DrawArraysIndirectCmd),
            static_cast<void*>(_draw_cmds.data())
        );

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _dibo_id);
        glBindVertexArray(_vao_id);

        glMultiDrawArraysIndirect(
            GL_TRIANGLES, 
            static_cast<void*>(0), 
            static_cast<GLsizei>(_draw_cmds.size()),
            sizeof(DrawArraysIndirectCmd)
        );
    }
}

void ChunkMeshPool::deinit() noexcept {
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &_vao_id);
    _vao_id = 0U;

    glUnmapNamedBuffer(_vbo_id);
    _vbo_ptr = nullptr;

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    u32 tmp[2] = { _vbo_id, _dibo_id };
    glDeleteBuffers(2, tmp);
    _vbo_id = 0U;
    _dibo_id = 0U;

    _free_chunks.clear();
    _draw_cmds.clear();
    _chunk_metadata.clear();
}