#ifndef VE001_CHUNK_GENERATOR_H
#define VE001_CHUNK_GENERATOR_H

#include <span>
#include <optional>

#include <vmath/vmath.h>

namespace ve001 {

struct ChunkGenerator {
    /// @brief initializes data of the generator thread-wise
    /// @return true if initialization failed
    virtual bool threadInit() noexcept = 0;
    /// @brief generates data at <chunk_position>
    /// @return generated data, if fails just return std::nullopt
    virtual std::optional<std::span<const vmath::u16>> gen(vmath::Vec3i32 chunk_position) noexcept = 0;    
};

}

#endif