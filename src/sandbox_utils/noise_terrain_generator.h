#ifndef VE001_NOISE_TERRAIN_GENERATOR_H
#define VE001_NOISE_TERRAIN_GENERATOR_H

#include "engine/chunk_generator.h"

#include <vector>
#include <FastNoise/FastNoise.h>

namespace ve001 {

struct NoiseTerrainGenerator : public ChunkGenerator {
    static constexpr std::size_t BUFFERS_COUNT{ 2UL };

    static thread_local std::vector<vmath::f32> _tmp_noise;
    static thread_local std::array<std::vector<vmath::u16>, BUFFERS_COUNT> _noise_buffers;
    static thread_local vmath::u32 _current_buffer;
    static thread_local FastNoise::SmartNode<> _smart_node;
    struct Config {
        /// @brief Size of single piece of terrain
        /// generated per iteration
        vmath::Vec3i32 terrain_size;
        /// @brief Noise frequency
        vmath::f32 noise_frequency;
        /// @brief Generated terrain is in discrete integer values
        /// It specifies max number of states to which to translate to.
        vmath::u32 quantize_values;
        /// @brief seed to generate random gradients
        vmath::u32 seed;
        /// @brief everything below this threshold will be clamped
        /// to 0. Must be in range (0.0, 1.0)
        vmath::f32 visibilty_threshold;
    };
    Config _config;

    NoiseTerrainGenerator(Config config);

    void threadInit() override;
    std::optional<std::span<const vmath::u16>> gen(vmath::Vec3i32 chunk_position) override;
};

}

#endif