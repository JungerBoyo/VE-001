#include "engine.h"

using namespace vmath;
using namespace ve001;

Engine::Engine(Vec3i32 chunk_size) : _engine_context(EngineContext{
    .shader_repo = {},
    .chunk_size = chunk_size,
    .chunk_size_1D = static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]),
    .chunk_voxel_data_size  = static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(u16),
    .chunk_max_mesh_size    = static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(Vertex) * static_cast<u64>(36/2),
    .chunk_max_submesh_size = static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(Vertex) * static_cast<u64>((36/2)/6),
    .meshing_axis_progress_step = 32
})
{}

void Engine::init(i32 view_distance) {
    _engine_context.shader_repo.init();
    _chunk_pool.init((2 * view_distance + 1) * (2 * view_distance + 1));
}
void Engine::deinit() {
    _chunk_pool.deinit();
    _engine_context.shader_repo.deinit();
}