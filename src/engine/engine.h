#ifndef VE001_ENGINE_H
#define VE001_ENGINE_H

#include <vmath/vmath.h>

#include "engine_context.h"
#include "shader_repository.h"
// #include "chunk_pool.h"
#include "world_grid.h"
#include "vertex.h"

namespace ve001 {

struct Engine {
    EngineContext _engine_context;
    // ChunkPool _chunk_pool{ _engine_context };
    WorldGrid _world_grid;

    Engine(vmath::Vec3f32 world_size, vmath::Vec3f32 initial_position, vmath::Vec3i32 chunk_size);
    void init();
    void deinit();
};

}

#endif