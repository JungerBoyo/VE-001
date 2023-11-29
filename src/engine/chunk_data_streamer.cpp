#include "chunk_data_streamer.h"

using namespace ve001;
using namespace vmath;

ChunkDataStreamer::ChunkDataStreamer(std::unique_ptr<ChunkGenerator> chunk_generator, std::size_t capacity)
    : _chunk_generator(std::move(chunk_generator)), _gen_promises(capacity) {
    try {
        const auto threads_count = std::thread::hardware_concurrency() == 0 ? 1 : std::thread::hardware_concurrency() - 2;
        for (std::uint32_t i{ 0U }; i < threads_count; ++i) {
            _threads.push_back(std::jthread(&ChunkDataStreamer::thread, this));
        }
    } catch([[maybe_unused]] const std::exception&) {
        _done = true;
    }
}

void ChunkDataStreamer::thread() {
    _chunk_generator->threadInit();
    while (!_done) {
        Promise promise;
        if (_gen_promises.read(promise)) {
            promise.value.set_value(_chunk_generator->gen(promise.position));
        } else {
            std::this_thread::yield();
        }
    }
}

std::future<std::optional<std::span<const vmath::u16>>> ChunkDataStreamer::gen(Vec3i32 chunk_position) {
    std::promise<std::optional<std::span<const vmath::u16>>> promise;
    std::future<std::optional<std::span<const vmath::u16>>> result(promise.get_future());

    _gen_promises.write({std::move(promise), chunk_position});

    return result;
}
