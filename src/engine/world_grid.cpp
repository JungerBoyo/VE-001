#include "world_grid.h"

using namespace vmath;
using namespace ve001;

WorldGrid::WorldGrid(const EngineContext& engine_context, Vec3f32 world_resolution, Vec3f32 initial_position) {
    // const Vec3f32 p0(0.F);
    // const Vec3f32 p1{ Vec3f32::divScalar(world_resolution, 2.F) };
 
    Vec3i32 grid_resolution{
       static_cast<i32>(world_resolution[0] / static_cast<f32>(engine_context.chunk_size[0])),
       static_cast<i32>(world_resolution[1] / static_cast<f32>(engine_context.chunk_size[1])),
       static_cast<i32>(world_resolution[2] / static_cast<f32>(engine_context.chunk_size[2])),
    };
}