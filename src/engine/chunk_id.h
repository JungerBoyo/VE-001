#ifndef VE001_CHUNK_ID_H
#define VE001_CHUNK_ID_H

#include <vmath/vmath_types.h>
#include <numeric>

namespace ve001 {

using ChunkId = vmath::u32;
constexpr ChunkId INVALID_CHUNK_ID{ std::numeric_limits<ChunkId>::max() };

}

#endif