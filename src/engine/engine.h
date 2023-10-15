#ifndef VE001_ENGINE_H
#define VE001_ENGINE_H

#include <vmath/vmath.h>

#include "engine_context.h"
#include "shader_repository.h"
#include "chunk_pool.h"
#include "vertex.h"

namespace ve001 {

struct Engine {
    EngineContext _engine_context;
    ChunkPool _chunk_pool{ _engine_context };

    Engine(vmath::Vec3i32 chunk_size);
    void init(vmath::i32 view_distance);
    void deinit();
};

}

#endif