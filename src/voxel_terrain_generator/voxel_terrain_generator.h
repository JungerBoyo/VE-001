#ifndef VE001_TERRAIN_GENERATOR_H
#define VE001_TERRAIN_GENERATOR_H

#include <functional>
#include <span>
#include <thread>
#include <atomic>
#include <future>
#include <optional>

#include <vmath/vmath.h>
#include <FastNoise/FastNoise.h>

#include <threadsafe_ringbuffer.h>

namespace ve001 {

struct VoxelTerrainGenerator {
    struct Promise {
        std::promise<std::optional<std::span<const vmath::u16>>> gen_result_promise;
        vmath::Vec3i32 position;
    };

    std::atomic_bool _done;
    std::vector<std::jthread> _threads;
    ThreadSafeRingBuffer<Promise> _gen_terrain_promises;

    static thread_local std::vector<vmath::f32> _tmp_noise;
    static thread_local std::vector<vmath::u16> _noise_double_buffer[2];
    static thread_local vmath::u32 _current_buffer;
    static thread_local FastNoise::SmartNode<> _smart_node;

    struct Config {
        enum class NoiseType {
            PERLIN,
            SIMPLEX
        };
        /**
         * @brief Size of single piece of terrain
         * generated per iteration
        */
        vmath::Vec3i32 terrain_size;
        /**
         * @brief Type of noise used for terrain generation
        */
        NoiseType noise_type;
        /**
         * @brief Noise frequency
        */
        vmath::f32 noise_frequency;
        /**
         * @brief Generated terrain is in discrete integer values
         * . It specifies max number of states to which to translate to.
        */
        vmath::u32 quantize_values;
        /**
         * @brief seed to generate random gradients
        */
        vmath::u32 seed;
    };

    Config _config;

    VoxelTerrainGenerator(Config config);

    void thread();
    /**
     * @brief generates next terrain chunk
     * @param chunk_position discrete position of the chunk in chunk extents
     * @return future to pointer to generated terrain, empty if terrain is all 0
    */
    std::future<std::optional<std::span<const vmath::u16>>> gen(vmath::Vec3i32 chunk_position);

    ~VoxelTerrainGenerator() { _done = true; }
};

}

#endif