#ifndef VE001_CHUNK_DATA_STREAMER_H
#define VE001_CHUNK_DATA_STREAMER_H

#include <span>
#include <thread>
#include <atomic>
#include <future>
#include <optional>
#include <memory>

#include <vmath/vmath.h>
#include "threadsafe_ringbuffer.h"

#include "chunk_generator.h"

namespace ve001 {

struct ChunkDataStreamer {
    struct Promise {
        std::promise<std::optional<std::span<const vmath::u16>>> value;
        vmath::Vec3i32 position;
    };

    std::atomic_bool _done;
    std::vector<std::jthread> _threads;
    ThreadSafeRingBuffer<Promise> _gen_promises;

    std::unique_ptr<ChunkGenerator> _chunk_generator;

    ChunkDataStreamer(vmath::u32 threads_count, std::unique_ptr<ChunkGenerator> chunk_generator, std::size_t capacity);

    void thread();
    /**
     * @brief generates chunk
     * @param chunk_position discrete position of the chunk in chunk extents
     * @return future to pointer to generated chunk, nullopt if chunk is all 0
    */
    std::future<std::optional<std::span<const vmath::u16>>> gen(vmath::Vec3i32 chunk_position);

    ~ChunkDataStreamer() { _done = true; }

};

}

#endif