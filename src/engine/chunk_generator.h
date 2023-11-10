#ifndef VE001_CHUNK_GENERATOR_H
#define VE001_CHUNK_GENERATOR_H

#include <span>
#include <optional>

#include <vmath/vmath.h>

namespace ve001 {

struct ChunkGenerator {
    virtual void threadInit() = 0;
    virtual std::optional<std::span<const vmath::u16>> gen(vmath::Vec3i32 chunk_position) = 0;    
};

}

#endif