#include "chunk_data_streamer.h"

#include <iostream>

using namespace ve001;
using namespace vmath;

ChunkDataStreamer::ChunkDataStreamer(EngineContext& engine_context, u32 threads_count, std::unique_ptr<ChunkGenerator> chunk_generator, std::size_t capacity) noexcept
    : _engine_context(engine_context), _chunk_generator(std::move(chunk_generator)) {

    try {
        _gen_promises.resize(capacity);
    } catch(const std::exception&) {
        _engine_context.error |= Error::CPU_ALLOCATION_FAILED;
        return;
    }

    try {
        auto final_threads_count = threads_count;
        if (final_threads_count == 0U) {
            final_threads_count = std::thread::hardware_concurrency() == 0 ? 1 : std::thread::hardware_concurrency();
        }
        for (std::uint32_t i{ 0U }; i < final_threads_count; ++i) {
			auto t = std::jthread(&ChunkDataStreamer::thread, this);
			std::cout << "Hello I'm thread " << t.get_id() << ", my job is generating" << std::endl;
            _threads.push_back(std::move(t));
        }
    } catch(const std::exception&) {
        _engine_context.error |= Error::CHUNK_DATA_STREAMER_THREAD_ALLOCATION_FAILED;
        _done = true;
    }
}

void ChunkDataStreamer::thread() noexcept {
    if (_done) {
        return;
    }
    if (_chunk_generator->threadInit()) {
		if (!_done) {
        	_done = true;
        	_engine_context.error |= Error::CHUNK_DATA_STREAMER_THREAD_INITIALIZATION_FAILED;
		}
        return;
    }

    ++_ready_counter;

    while (!_done) {
        Promise promise;
        if (_gen_promises.read(promise)) {
            // can throw but will never happen in practice
            promise.value.set_value(_chunk_generator->gen(promise.position));
        } else {
            std::this_thread::yield();
        }
    }
}

std::future<std::optional<std::span<const vmath::u16>>> ChunkDataStreamer::gen(Vec3i32 chunk_position) noexcept {
    // can throw but will never happen in practice
    std::promise<std::optional<std::span<const vmath::u16>>> promise;
    std::future<std::optional<std::span<const vmath::u16>>> result(promise.get_future());
    
    /// will never return false since size == max chunks count
    _gen_promises.write(std::move(promise), chunk_position);

    return result;
}



