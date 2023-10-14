#ifndef VE001_CHUNK_DATA_POOL_H
#define VE001_CHUNK_DATA_POOL_H

#include <vector>
#include <concepts>

namespace ve001 {

template<typename T>
requires std::is_unsigned_v<T>
struct ChunkDataPool {
    std::vector<T> data;

     
};

}

#endif