#include "chunk_pool.h"

#include <glad/glad.h>

#include <cstring>
#include <iostream>

using namespace ve001;
using namespace vmath;

#define VE001_SH_CONFIG_ATTRIB_INDEX_POSITION 0
#define VE001_SH_CONFIG_ATTRIB_INDEX_TEXCOORD 1

static void setVertexLayout(u32 vao, u32 vbo) {
    constexpr u32 vertex_attrib_binding = 0U;

    glEnableVertexArrayAttrib(vao, VE001_SH_CONFIG_ATTRIB_INDEX_POSITION);
    glEnableVertexArrayAttrib(vao, VE001_SH_CONFIG_ATTRIB_INDEX_TEXCOORD);

    glVertexArrayAttribFormat(vao, VE001_SH_CONFIG_ATTRIB_INDEX_POSITION, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribFormat(vao, VE001_SH_CONFIG_ATTRIB_INDEX_TEXCOORD, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, texcoord));
    
    glVertexArrayAttribBinding(vao, VE001_SH_CONFIG_ATTRIB_INDEX_POSITION, vertex_attrib_binding);
    glVertexArrayAttribBinding(vao, VE001_SH_CONFIG_ATTRIB_INDEX_TEXCOORD, vertex_attrib_binding);

    glVertexArrayBindingDivisor(vao, vertex_attrib_binding, 0);

    glVertexArrayVertexBuffer(vao, vertex_attrib_binding, vbo, 0, sizeof(Vertex));
}

void ChunkPool::init(i32 max_chunks) noexcept {
    _chunks_count = max_chunks;

    u32 tmp[3] = { 0U, 0U, 0U };

    glCreateBuffers(3, tmp);
    _vbo_id = tmp[0];
    _ibo_id = tmp[1];
    _dibo_id = tmp[2];

    glNamedBufferStorage(_vbo_id, static_cast<i64>(_chunks_count) * static_cast<i64>(_engine_context.chunk_max_mesh_size), nullptr, 0);

    glCreateVertexArrays(1, &_vao_id);
    setVertexLayout(_vao_id, _vbo_id);

    glNamedBufferStorage(
        _dibo_id,
        6 * max_chunks * sizeof(DrawElementsIndirectCmd),
        nullptr,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    _dibo_mapped_ptr = glMapNamedBufferRange(
        _dibo_id, 
        0, 6 * max_chunks * sizeof(DrawElementsIndirectCmd),
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    try {
        _chunks.reserve(max_chunks);
        _chunk_id_to_index.resize(max_chunks, INVALID_CHUNK_INDEX);
        _voxel_data.resize(static_cast<u64>(max_chunks) * _engine_context.chunk_size_1D);
        _free_chunks = RingBuffer<FreeChunk>(_chunks_count, {});
        _draw_cmds.reserve(_chunks_count * 6);
        for (std::size_t i{ 0U }; i < _chunks_count; ++i) {
            _free_chunks.write({
                .chunk_id = static_cast<u32>(i),
                .gpu_region_offset = i * _engine_context.chunk_max_mesh_size,
                .cpu_region = std::span<u16>(_voxel_data.data() + i * _engine_context.chunk_size_1D, _engine_context.chunk_size_1D)
            });
        }

        static constexpr u32 INDICES_PATTERN[6] = { 0, 1, 2, 0, 2, 3 };
        std::vector<u32> indices(_engine_context.chunk_max_submesh_indices_size/sizeof(u32));
        for (std::size_t i{ 0UL }, q{ 0UL }; i < indices.size(); i += 6, q += 4) {
            indices[i + 0] = INDICES_PATTERN[0] + static_cast<u32>(q);
            indices[i + 1] = INDICES_PATTERN[1] + static_cast<u32>(q);
            indices[i + 2] = INDICES_PATTERN[2] + static_cast<u32>(q);
            indices[i + 3] = INDICES_PATTERN[3] + static_cast<u32>(q);
            indices[i + 4] = INDICES_PATTERN[4] + static_cast<u32>(q);
            indices[i + 5] = INDICES_PATTERN[5] + static_cast<u32>(q);
        }
        glNamedBufferStorage(_ibo_id, static_cast<i64>(_engine_context.chunk_max_submesh_indices_size), static_cast<const void*>(indices.data()), 0);

    } catch ([[maybe_unsused]] const std::exception& e) {
        glDeleteBuffers(3, tmp);
        glDeleteVertexArrays(1, &_vao_id);
        return; // TODO: Error
    }

    _meshing_engine.init(_vbo_id);
}

u32 ChunkPool::allocateChunk(const std::function<void(void*)>& voxel_write_data, Vec3i32 position) noexcept {
    if (_free_chunks.empty()) {
        return INVALID_CHUNK_ID; // TODO: Error
    }

    FreeChunk free_chunk{};
    _free_chunks.read(free_chunk);

    voxel_write_data(static_cast<void*>(free_chunk.cpu_region.data()));

    auto& chunk = _chunks.emplace_back(Chunk{
        Vec3f32::cast(Vec3i32::sub(Vec3i32::mul(position, _engine_context.chunk_size), _engine_context.half_chunk_size)), 
        {0U, 0U, 0U, 0U, 0U, 0U}, // bcs chunk isn't complete yet
        free_chunk.gpu_region_offset,
        free_chunk.cpu_region,
        free_chunk.chunk_id,
        false
    });

    _chunk_id_to_index[chunk.chunk_id] = _chunks.size() - 1;

    _meshing_engine.issueMeshingCommand(
        chunk.chunk_id, position,
        chunk.cpu_region, 
        chunk.gpu_region_offset
    );

    return chunk.chunk_id;
}

vmath::u32 ChunkPool::allocateChunk(std::span<const vmath::u16> src, Vec3i32 position) noexcept {
    if (_free_chunks.empty()) {
        return INVALID_CHUNK_ID; // TODO: Error
    }

    FreeChunk free_chunk{};
    _free_chunks.read(free_chunk);

    std::memcpy(static_cast<void*>(free_chunk.cpu_region.data()), static_cast<const void*>(src.data()), src.size() * sizeof(u16));

    cpu_memory_usage += src.size() * sizeof(u16);

    auto& chunk = _chunks.emplace_back(Chunk{
        Vec3f32::cast(Vec3i32::sub(Vec3i32::mul(position, _engine_context.chunk_size), _engine_context.half_chunk_size)), 
        {0U, 0U, 0U, 0U, 0U, 0U}, // bcs chunk isn't complete yet
        free_chunk.gpu_region_offset,
        free_chunk.cpu_region,
        free_chunk.chunk_id,
        false
    });

    _chunk_id_to_index[chunk.chunk_id] = _chunks.size() - 1;

    _meshing_engine.issueMeshingCommand(
        free_chunk.chunk_id, position,
        free_chunk.cpu_region, 
        free_chunk.gpu_region_offset
    );

    return chunk.chunk_id;
}

void ChunkPool::completeChunk(MeshingEngine::Result result) {
    const auto chunk_index = _chunk_id_to_index[result.chunk_id];
    if (chunk_index == INVALID_CHUNK_INDEX) {
        return;
    }
    auto& chunk = _chunks[chunk_index];
    chunk.complete = true;
    const auto base_cmd_index = static_cast<u32>(_draw_cmds.size());
    chunk.draw_cmd_indices[X_POS] = base_cmd_index + X_POS;
    chunk.draw_cmd_indices[X_NEG] = base_cmd_index + X_NEG;
    chunk.draw_cmd_indices[Y_POS] = base_cmd_index + Y_POS;
    chunk.draw_cmd_indices[Y_NEG] = base_cmd_index + Y_NEG;
    chunk.draw_cmd_indices[Z_POS] = base_cmd_index + Z_POS;
    chunk.draw_cmd_indices[Z_NEG] = base_cmd_index + Z_NEG;

    for (std::size_t i{ 0U }; i < 6U; ++i) {
        const auto& draw_cmd = _draw_cmds.emplace_back(DrawElementsIndirectCmd{
            .count = result.written_indices[i],
            .instance_count = 1U,
            .first_index = 0U,
            .base_vertex =  static_cast<i32>((((static_cast<u64>(result.chunk_id) * 6UL) + i) * _engine_context.chunk_max_submesh_size)/sizeof(Vertex)),
            .base_instance = 0U,
            .orientation = static_cast<Face>(i),
            .chunk_id = result.chunk_id
        });
        gpu_memory_usage += static_cast<u64>(result.written_indices[i]/6) * sizeof(Vertex) * 4;
    }
    _draw_cmds_dirty = true;
}

void ChunkPool::update(bool use_partition) {
    if (_draw_cmds.size() > 0U) {
        if (use_partition && _draw_cmds_parition_size == 0U) {
            return;
        }
        if (_draw_cmds_dirty) {
            const auto size = (use_partition ? _draw_cmds_parition_size : _draw_cmds.size()) * sizeof(DrawElementsIndirectCmd);
            std::memcpy(_dibo_mapped_ptr, static_cast<const void*>(_draw_cmds.data()), size);
            _draw_cmds_dirty = false;
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _dibo_id);
        }
        glBindVertexArray(_vao_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo_id);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);
    }
}
void ChunkPool::drawAll(bool use_partition) {
    if (_draw_cmds.size() > 0U) {
        if (use_partition && _draw_cmds_parition_size == 0U) {
            return;
        }
        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            nullptr,
            use_partition ? _draw_cmds_parition_size : _draw_cmds.size(),
            sizeof(DrawElementsIndirectCmd)
        );
    }
}
bool ChunkPool::poll() {
    if (MeshingEngine::Result result{}; _meshing_engine.pollMeshingCommand(result)) {
        completeChunk(result);
        return true;
    }
    return false;
}

void ChunkPool::deallocateChunk(u32 chunk_id) noexcept {
    if (chunk_id == INVALID_CHUNK_ID) {
        return;
    }

    const auto chunk_index = _chunk_id_to_index[chunk_id];
    if (chunk_index == INVALID_CHUNK_INDEX) {
        return;
    }
    const auto chunk = _chunks[chunk_index];


    if (chunk.complete) {
        deallocateChunkDrawCommands(chunk_id);
    }

    if (chunk_index != _chunks.size() - 1U) {
        const auto last_chunk = _chunks.back();
        _chunks[chunk_index] = last_chunk;
        _chunk_id_to_index[last_chunk.chunk_id] = chunk_index;
    }

    
    _chunk_id_to_index[chunk_id] = INVALID_CHUNK_INDEX;
    _free_chunks.write({
        .chunk_id = chunk_id,
        .gpu_region_offset = chunk.gpu_region_offset,
        .cpu_region = chunk.cpu_region
    });
    _chunks.pop_back();
}
void ChunkPool::deallocateChunkDrawCommands(ChunkId chunk_id) {
    const auto& chunk = _chunks[_chunk_id_to_index[chunk_id]];
    for (u32 i{ 0U }; i < 6; ++i) {
        const auto draw_cmd_index = chunk.draw_cmd_indices[i];
        gpu_memory_usage -= static_cast<u64>(_draw_cmds[draw_cmd_index].count/6) * sizeof(Vertex) * 4;
        if (draw_cmd_index != _draw_cmds.size() - 1U) {
            auto& last_draw_cmd = _draw_cmds.back();
            const auto last_draw_cmd_chunk_index = _chunk_id_to_index[last_draw_cmd.chunk_id];
            auto& last_draw_cmd_chunk = _chunks[last_draw_cmd_chunk_index];

            const auto last_draw_cmd_submesh_index = static_cast<u32>(last_draw_cmd.orientation);
            last_draw_cmd_chunk.draw_cmd_indices[last_draw_cmd_submesh_index] = draw_cmd_index;

            _draw_cmds[draw_cmd_index] = last_draw_cmd;
        }
        _draw_cmds.pop_back();
    }
    _draw_cmds_dirty = true;
}

void ChunkPool::forceCommandsDirty() {
    _draw_cmds_dirty = true;
}

void ChunkPool::deinit() {
    _meshing_engine.deinit();

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &_vao_id);
    _vao_id = 0U;

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    glUnmapNamedBuffer(_dibo_id);
    _dibo_mapped_ptr = nullptr;

    u32 tmp[3] = { _vbo_id, _dibo_id, _ibo_id };
    glDeleteBuffers(3, tmp);
    _vbo_id = 0U;
    _dibo_id = 0U;
    _ibo_id = 0U;

    _free_chunks.clear();
    _draw_cmds.clear();
    _chunks.clear();
}