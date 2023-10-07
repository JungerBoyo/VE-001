#ifndef VE001_TERRAIN_BUFFER_H
#define VE001_TERRAIN_BUFFER_H

#include <concepts>
#include <vector>

namespace ve001 {

template<typename T>
requires (std::is_unsigned_v<T> && !std::is_same_v<T, bool>)
struct Chunk {
    std::vector<T> data;
};

struct ChunkBuffer {
    
};

}

#endif