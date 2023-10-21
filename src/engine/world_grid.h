#ifndef VE001_WORLD_GRID_H
#define VE001_WORLD_GRID_H

#include <vmath/vmath.h>

#include "engine_context.h"

namespace ve001 {

struct WorldGrid {
    WorldGrid(const EngineContext& engine_context, vmath::Vec3f32 world_resolution, vmath::Vec3f32 initial_position);
};

}

#endif