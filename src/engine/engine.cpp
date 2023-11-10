#include "engine.h"

#include "noise_terrain_generator.h"

using namespace vmath;
using namespace ve001;

Engine::Engine(Vec3f32 world_size, Vec3f32 initial_position, Vec3i32 chunk_size) : _engine_context(EngineContext{
    .shader_repo = {},
    .chunk_size = chunk_size,
    .half_chunk_size = Vec3i32::divScalar(chunk_size, 2),
    .chunk_size_1D = static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]),
    .chunk_voxel_data_size  = static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(u16),
    .chunk_max_mesh_size    = (static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(Vertex) * static_cast<u64>(36/2))/32,
    .chunk_max_submesh_size = ((static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(Vertex) * static_cast<u64>((36/2))/32)/6),
    .meshing_axis_progress_step = 64
}),
  _world_grid(_engine_context, world_size, initial_position, std::unique_ptr<ChunkGenerator>(new NoiseTerrainGenerator({
        .terrain_size = chunk_size,
        .noise_frequency = .01F,
        .quantize_values = 1U,
        .seed = 0xC0000B99
  })))
{}

void Engine::init() {
    _engine_context.shader_repo.init();
    _world_grid.init();
}
void Engine::deinit() {
    _world_grid.deinit();
    _engine_context.shader_repo.deinit();
}