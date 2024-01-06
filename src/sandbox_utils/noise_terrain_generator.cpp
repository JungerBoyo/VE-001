#include "noise_terrain_generator.h"

using namespace ve001;
using namespace vmath;

thread_local std::vector<vmath::f32> NoiseTerrainGenerator::_tmp_noise;
thread_local std::array<std::vector<vmath::u16>, NoiseTerrainGenerator::BUFFERS_COUNT> NoiseTerrainGenerator::_noise_buffers;
thread_local vmath::u32 NoiseTerrainGenerator::_current_buffer;
thread_local FastNoise::SmartNode<> NoiseTerrainGenerator::_smart_node;

NoiseTerrainGenerator::NoiseTerrainGenerator(Config config) noexcept
    : _config(config) {
    _config.visibilty_threshold = std::clamp(_config.visibilty_threshold, 0.F, 1.F);        
}

bool NoiseTerrainGenerator::threadInit() noexcept {
    _current_buffer = 0U;
    try {
        _tmp_noise.resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0.F);
        for (auto& buffer : _noise_buffers) {
            buffer.resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0U);
        }
        _smart_node = FastNoise::NewFromEncodedNodeTree(
            "IQAZABAAexQoQA0AAwAAAAAAAEAIAAAAAAA/AAAAAAABAwCPwnU9AQQAAAAAANejCMEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD//wEAAClcjz4="
        ); 
    } catch(const std::exception&) {
        return true;
    }
    return false;
}
std::optional<std::span<const vmath::u16>> NoiseTerrainGenerator::gen(vmath::Vec3i32 chunk_position) noexcept {
    const auto p0 = Vec3i32::mul(chunk_position, _config.terrain_size);
    const auto p1 = _config.terrain_size;

    _smart_node->GenUniformGrid3D(
        _tmp_noise.data(), 
        p0[0], p0[1], p0[2], p1[0], p1[1], p1[2], 
        _config.noise_frequency, _config.seed
    );

    auto& buffer = _noise_buffers[_current_buffer];

    std::size_t i{ 0UL };
    u32 not_empty{ 0UL };
    for (const auto noise_value : _tmp_noise) {
        auto fvalue = ((noise_value + 1.F)/2.F);

        u16 value{ 0U };
        if (fvalue >= _config.visibilty_threshold) {
            const auto stretched_upper_fvalue = ((1.F - fvalue) + _config.visibilty_threshold);
            value = std::clamp(static_cast<u32>(stretched_upper_fvalue * static_cast<f32>(_config.quantize_values)), 0U, _config.quantize_values);
        }

        not_empty += static_cast<u32>(value > 0);
        buffer[i++] = value;
    }
    if (not_empty) {
        _current_buffer = (_current_buffer + 1) % _noise_buffers.size();
    }

    return not_empty ? std::optional(std::span<const u16>(buffer)) : std::nullopt;
}
