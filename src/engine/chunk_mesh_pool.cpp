#include "chunk_mesh_pool.h"

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
    
    glCreateBuffers(1, &_vbo_id);
    glNamedBufferStorage(
        _vbo_id,
        _max_chunk_size * config.max_chunks,
        nullptr,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    glCreateVertexArrays(1, &_vao_id);
    config.vertex_layout_config(_vao_id, _vbo_id);

    _vbo_ptr = glMapNamedBuffer(
        _vbo_id,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    if (_vbo_ptr == nullptr) {
        return Error::FAILED_TO_MAP_VBO_BUFFER;
    }

    _chunks_count = config.max_chunks;
    _submeshes_count = 6 * config.max_chunks;
    _vertex_size = config.vertex_size;

    try {
        _free_pool_regions = RingBuffer<void*>(_chunks_count, nullptr);
        for (std::size_t c{ 0U }; c < _chunks_count; ++c) {
            _free_pool_regions.pushBack(_vbo_ptr + c * _max_chunk_size);
        }
    } catch (const std::bad_alloc&) {
        return Error::CPU_MEMORY_ALLOCATION_FAILED;
    }

    return Error::NO_ERROR;
}

Error ChunkMeshPool::allocateChunk(i32& chunk_id, ChunkData chunk_data, Vec3f32 position) noexcept {
    chunk_id = INT32_MAX; 

    if (chunk_data.size == 0U || chunk_data.size > _max_chunk_size) {
        return Error::INVALID_CHUNK_SIZE;
    }

    if (_free_pool_regions.empty()) {
        return Error::NO_FREE_CHUNKS;
    }

    const auto _chunk_id = _chunk_submesh_metadata.size();
    _chunk_submesh_metadata.emplace_back(position, _chunk_id);

    for (std::size_t i{ 0U }; i < 6U; ++i) {
        auto& submesh = chunk_data.submesh_data[i];
        if (submesh.vertex_count == 0U || submesh.vertex_count * _vertex_size > _max_submesh_size) {
            return Error::INVALID_SUBMESH_SIZE;
        }
        if (submesh.ptr == nullptr) {
            return Error::INVALID_SUBMESH_PTR;
        }

        void* region_offset{ nullptr };
        _free_pool_regions.popBack(region_offset);

        _draw_cmds.emplace_back(
            submesh.vertex_count, 
            1U, 
            static_cast<u8*>(region_offset) - static_cast<u8*>(_vbo_ptr) + i * _max_submesh_size, 
            0U, 
            static_cast<SubmeshOrientation>(i),
            _chunk_id
        );
    }

    chunk_id = _chunk_id;

    return Error::NO_ERROR;
}