#include "noise_terrain_generator.h"

using namespace ve001;
using namespace vmath;

thread_local std::vector<vmath::f32> NoiseTerrainGenerator::_tmp_noise;
thread_local std::vector<vmath::u16> NoiseTerrainGenerator::_noise_double_buffer[2];
thread_local vmath::u32 NoiseTerrainGenerator::_current_buffer;
thread_local FastNoise::SmartNode<> NoiseTerrainGenerator::_smart_node;

NoiseTerrainGenerator::NoiseTerrainGenerator(Config config) 
    : _config(config) {
    _config.visibilty_threshold = std::clamp(_config.visibilty_threshold, 0.F, 1.F);        
}

void NoiseTerrainGenerator::threadInit() {
    _current_buffer = 0U;
    _tmp_noise.resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0.F);
    _noise_double_buffer[0].resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0U);
    _noise_double_buffer[1].resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0U);
    // "IQAZABAAexQoQA0AAwAAAAAAAEAIAAAAAAA/AAAAAAABAwCPwnU9AQQAAAAAAArXo78AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD//wEAAClcjz4="
    // "IQAZABAAexQoQA0AAwAAAAAAAEAIAAAAAAA/AAAAAAABAwCPwnU9AQQAAAAAALgeVcEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD//wEAAB+Fqz8="
    // "EQACAAAAAAAgQBAAAAAAQBkAEwDD9Sg/DQAEAAAAAAAgQAkAAGZmJj8AAAAAPwEEAAAAAAAAAEBAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAM3MTD4AMzMzPwAAAAA/"
    _smart_node = FastNoise::NewFromEncodedNodeTree(
        "IQAZABAAexQoQA0AAwAAAAAAAEAIAAAAAAA/AAAAAAABAwCPwnU9AQQAAAAAANejCMEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD//wEAAClcjz4="
        // "IQAZABAAexQoQA0AAwAAAAAAAEAIAAAAAAA/AAAAAAABAwCPwnU9AQQAAAAAALgeVcEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD//wEAAOxROD8="
        // "GgAAAAAAAAERAAIAAAAAACBAEAAAAABAGQATAMP1KD8NAAQAAAAAACBACQAAZmYmPwAAAAA/AQQAAAAAAAAAQEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAzcxMPgBI4Xo/AAAAAD8="
    );
}
std::optional<std::span<const vmath::u16>> NoiseTerrainGenerator::gen(vmath::Vec3i32 chunk_position) {
    const auto p0 = Vec3i32::mul(chunk_position, _config.terrain_size);
    const auto p1 = _config.terrain_size;

    _smart_node->GenUniformGrid3D(
        _tmp_noise.data(), 
        p0[0], p0[1], p0[2], p1[0], p1[1], p1[2], 
        _config.noise_frequency, _config.seed
    );

    auto& buffer = _noise_double_buffer[_current_buffer];

    std::size_t i{ 0UL };
    u32 not_empty{ 0UL };
    for (const auto noise_value : _tmp_noise) {
        const auto fvalue = ((noise_value + 1.F)/2.F);
        const u16 value = fvalue < _config.visibilty_threshold ? 0U :
            std::clamp(
                static_cast<u32>(((1.F - fvalue) + _config.visibilty_threshold) * static_cast<f32>(_config.quantize_values)),
                0U, _config.quantize_values
            );

        not_empty += static_cast<u32>(value > 0);
        buffer[i++] = value;
    }
    if (not_empty) {
        _current_buffer = _current_buffer ^ 0x1U;
    }

    return not_empty ? std::optional(std::span<const u16>(buffer)) : std::nullopt;
}
