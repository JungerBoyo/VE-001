#include "noise_terrain_generator.h"

using namespace ve001;
using namespace vmath;

thread_local std::vector<vmath::f32> NoiseTerrainGenerator::_tmp_noise;
thread_local std::vector<vmath::u16> NoiseTerrainGenerator::_noise_double_buffer[2];
thread_local vmath::u32 NoiseTerrainGenerator::_current_buffer;
thread_local FastNoise::SmartNode<> NoiseTerrainGenerator::_smart_node;

NoiseTerrainGenerator::NoiseTerrainGenerator(Config config) 
    : _config(config) {}

void NoiseTerrainGenerator::threadInit() {
    _current_buffer = 0U;
    _tmp_noise.resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0.F);
    _noise_double_buffer[0].resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0U);
    _noise_double_buffer[1].resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0U);
    
    // auto fn_simplex = FastNoise::New<FastNoise::Simplex>(FastSIMD::Level_AVX512);
    // auto fn_fractal = FastNoise::New<FastNoise::FractalFBm>(FastSIMD::Level_AVX512);
    // fn_fractal->SetSource(fn_simplex);
    // fn_fractal->SetGain(11.F);
    // fn_fractal->SetWeightedStrength(18.F);
    // fn_fractal->SetOctaveCount(2);
    // fn_fractal->SetLacunarity(-.19F);

    // auto fn_position_output = FastNoise::New<FastNoise::PositionOutput>(FastSIMD::Level_AVX512);
    // fn_position_output->Set<FastNoise::Dim::Y>(-2.56F, -.14F);
    // fn_position_output->Set<FastNoise::Dim::X>(-.42F, 0.F);

    // auto fn_add = FastNoise::New<FastNoise::Add>(FastSIMD::Level_AVX512);
    // fn_add->SetLHS(fn_fractal);
    // fn_add->SetRHS(fn_position_output);

    _smart_node = FastNoise::NewFromEncodedNodeTree(
        "IQAZABAAexQoQA0AAwAAAAAAAEAIAAAAAAA/AAAAAAABAwCPwnU9AQQAAAAAAArXo78AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD//wEAAClcjz4="
    );//fn_add;
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
        const auto fvalue = /*std::ceil(*/noise_value * static_cast<f32>(_config.quantize_values);//);
        const u16 value = fvalue > 1.3F ? 2U : static_cast<u16>(std::clamp(fvalue, 0.F, 1.F));
        // if (noise_value > .8F) {
            
        // }
        // const auto value = ;
        not_empty += static_cast<u32>(value > 0);
        buffer[i++] = value;
    }
    if (not_empty) {
        _current_buffer = _current_buffer ^ 0x1U;
    }

    return not_empty ? std::optional(std::span<const u16>(buffer)) : std::nullopt;
}
