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
#include "engine_context.h"
#include "chunk_generator.h"

namespace ve001 {

struct ChunkDataStreamer {
    struct Promise {
        std::promise<std::optional<std::span<const vmath::u16>>> value;
        vmath::Vec3i32 position;
    };
    /// @brief counts number of threads' which are already
    /// initialized
    std::atomic_int32_t _ready_counter{ 0 };

    EngineContext& _engine_context;

    /// @brief threads' exit condition
    std::atomic_bool _done{ false };
    /// @brief allocated threads
    std::vector<std::jthread> _threads;
    /// @brief promise queue for generation task
    ThreadSafeRingBuffer<Promise> _gen_promises{};
    /// @brief used chunk data generator ptr
    std::unique_ptr<ChunkGenerator> _chunk_generator;

    ChunkDataStreamer(EngineContext& engine_context, vmath::u32 threads_count, std::unique_ptr<ChunkGenerator> chunk_generator, std::size_t capacity) noexcept;

    /// @brief threads' entry point
    void thread() noexcept;

    /// @brief signals if chunk data streamer was fully initialized
    /// @return true if chunk data streamer is initialzed
    bool ready() const noexcept { return _ready_counter == _threads.size(); }

    /**
     * @brief generates chunk
     * @param chunk_position discrete position of the chunk in chunk extents
     * @return future to pointer to generated chunk, nullopt if chunk is all 0
    */
    std::future<std::optional<std::span<const vmath::u16>>> gen(vmath::Vec3i32 chunk_position) noexcept;

    ~ChunkDataStreamer() noexcept { _done = true; }

};

}

#endif
