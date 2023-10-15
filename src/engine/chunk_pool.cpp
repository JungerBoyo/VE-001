#include "chunk_pool.h"

#include <glad/glad.h>

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

    u32 tmp[2] = { 0U, 0U };

    glCreateBuffers(2, tmp);
    _vbo_id = tmp[0];
    _dibo_id = tmp[1];

    glNamedBufferStorage(_vbo_id, _chunks_count * _engine_context.chunk_max_mesh_size, nullptr, 0);

    glNamedBufferStorage(
        _dibo_id,
        6 * max_chunks * sizeof(DrawArraysIndirectCmd),
        nullptr,
        GL_DYNAMIC_STORAGE_BIT
    );
    glCreateVertexArrays(1, &_vao_id);
    setVertexLayout(_vao_id, _vbo_id);

    try {
        _chunks.reserve(max_chunks);
        _chunk_id_to_index.reserve(max_chunks);
        _voxel_data.resize(max_chunks * _engine_context.chunk_size_1D);
        _free_chunks = RingBuffer<FreeChunk>(_chunks_count, {});
        for (std::size_t i{ 0U }; i < _chunks_count; ++i) {
            _free_chunks.write({
                .chunk_id = static_cast<u32>(i),
                .gpu_region_offset = static_cast<u32>(i * _engine_context.chunk_max_mesh_size),
                .cpu_region = std::span<u16>(_voxel_data.data() + static_cast<i32>(i) * _engine_context.chunk_size_1D, _engine_context.chunk_size_1D)
            });
        }
    } catch ([[maybe_unsused]] const std::exception& e) {
        glDeleteBuffers(2, tmp);
        glDeleteVertexArrays(1, &_vao_id);
        return; // TODO: Error
    }

    _meshing_engine.init(_vbo_id);
}

u32 ChunkPool::allocateChunk(const std::function<void(void*)>& voxel_write_data, Vec3i32 position) noexcept {
    u32 chunk_id{ std::numeric_limits<u32>::max() };

    if (_free_chunks.empty()) {
        return chunk_id; // TODO: Error
    }

    FreeChunk free_chunk{};
    _free_chunks.read(free_chunk);

    voxel_write_data(static_cast<void*>(free_chunk.cpu_region.data()));

    auto& chunk = _chunks.emplace_back(Chunk{
        position, 
        {0}, // bcs chunk isn't complete yet
        free_chunk.gpu_region_offset,
        free_chunk.cpu_region,
        free_chunk.chunk_id,
        false
    });

    _chunk_id_to_index.push_back(chunk_id);

    _meshing_engine.issueMeshingCommand(
        chunk.chunk_id, position,
        chunk.cpu_region, 
        chunk.gpu_region_offset, _engine_context.chunk_max_mesh_size
    );
}

void ChunkPool::completeChunk(MeshingEngine::Future future) {
    const auto chunk_index = _chunk_id_to_index[future.chunk_id];
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
        _draw_cmds.emplace_back(DrawArraysIndirectCmd{
            .count = future.written_vertices_counts[i],
            .instance_count = 1U,
            .first = 0U,
            .base_instance = 0U,
            .orientation = static_cast<Face>(i),
            .chunk_id = future.chunk_id
        });
    }
}

void ChunkPool::update() {
    if (_draw_cmds.size() > 0U) {
        glNamedBufferSubData(
            _dibo_id, 0, 
            _draw_cmds.size() * sizeof(DrawArraysIndirectCmd),
            static_cast<void*>(_draw_cmds.data())
        );

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _dibo_id);
        glBindVertexArray(_vao_id);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_id);
    }
}
void ChunkPool::drawAll() {
    if (_draw_cmds.size() > 0U) {
        glMultiDrawArraysIndirect(
            GL_TRIANGLES, 
            static_cast<void*>(0), 
            static_cast<GLsizei>(_draw_cmds.size()),
            sizeof(DrawArraysIndirectCmd)
        );
    }
}
void ChunkPool::poll() {
    if (MeshingEngine::Future future{}; _meshing_engine.pollMeshingCommand(future)) {
        completeChunk(future);
    }
}

void ChunkPool::deallocateChunk(u32 chunk_id) noexcept {
    const auto chunk_index = _chunk_id_to_index[chunk_id];
    auto& chunk = _chunks[chunk_index];

    if (chunk.complete) {
        deallocateChunkDrawCommands(chunk);
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
void ChunkPool::deallocateChunkDrawCommands(const Chunk& chunk) {
    for (std::size_t i{ 0U }; i < 6U; ++i) {
        const auto draw_cmd_index = chunk.draw_cmd_indices[i];
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
}

void ChunkPool::deinit() {
    _meshing_engine.deinit();

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &_vao_id);
    _vao_id = 0U;

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    u32 tmp[2] = { _vbo_id, _dibo_id };
    glDeleteBuffers(2, tmp);
    _vbo_id = 0U;
    _dibo_id = 0U;

    _free_chunks.clear();
    _draw_cmds.clear();
    _chunks.clear();
}