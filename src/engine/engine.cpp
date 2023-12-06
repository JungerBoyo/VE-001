#include "engine.h"

#include "noise_terrain_generator.h"
#include "simple_terrain_generator.h"

using namespace vmath;
using namespace ve001;

// #define SIMPLE_GENERATOR
// #define MAX_DIV_FACTOR 1

static constexpr u64 MAX_DIV_FACTOR{ 32UL };

// #define NO_RESIZE

Engine::Engine(Vec3f32 world_size, Vec3f32 initial_position, Vec3i32 chunk_size) : _engine_context(EngineContext{
    .shader_repo = {},
    .chunk_size = chunk_size,
    .half_chunk_size = Vec3i32::divScalar(chunk_size, 2),
    .chunk_size_1D = static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]),
    .chunk_voxel_data_size  = static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(u16),
    .chunk_max_possible_submesh_indices_size = static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(u32) * static_cast<u64>(36/6/2),
    .chunk_max_possible_mesh_size    =  (static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(Vertex) * static_cast<u64>(24/2)),
    .chunk_max_possible_submesh_size = ((static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(Vertex) * static_cast<u64>(24/2))/6),
    // .chunk_max_current_submesh_indices_size = sizeof(u32) * static_cast<u64>(24/6),
#ifdef NO_RESIZE
    .chunk_max_current_mesh_size    =  (static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(Vertex) * static_cast<u64>(24/2)),
    .chunk_max_current_submesh_size = ((static_cast<u64>(chunk_size[0]) * static_cast<u64>(chunk_size[1]) * static_cast<u64>(chunk_size[2]) * sizeof(Vertex) * static_cast<u64>(24/2))/6),
#else
    .chunk_max_current_mesh_size = sizeof(Vertex) * static_cast<u64>(24),
    .chunk_max_current_submesh_size = sizeof(Vertex) * static_cast<u64>(24/6),
#endif
    .meshing_axis_progress_step = 64
}),
  _world_grid(_engine_context, world_size, initial_position, 
#ifdef SIMPLE_GENERATOR
  std::unique_ptr<ChunkGenerator>(new SimpleTerrainGenerator(chunk_size))
#else
  std::unique_ptr<ChunkGenerator>(new NoiseTerrainGenerator({
        .terrain_size = chunk_size,
        .noise_frequency = .008F,
        .quantize_values = 2U,
        .seed = 0xC0000B99
  }))
#endif 
  )
{


}

void Engine::init() {
    _engine_context.shader_repo.init();
    _world_grid.init();
}
void Engine::deinit() {
    _world_grid.deinit();
    _engine_context.shader_repo.deinit();
}