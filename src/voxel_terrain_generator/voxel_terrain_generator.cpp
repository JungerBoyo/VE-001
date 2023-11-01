#include "voxel_terrain_generator.h"

#include <cstring>
#include <cmath>

#include <iostream>

using namespace vmath;
using namespace ve001;

thread_local std::vector<vmath::f32> VoxelTerrainGenerator::_tmp_noise;
thread_local std::vector<vmath::u16> VoxelTerrainGenerator::_noise_double_buffer[2];
thread_local vmath::u32 VoxelTerrainGenerator::_current_buffer;
thread_local FastNoise::SmartNode<> VoxelTerrainGenerator::_smart_node;

VoxelTerrainGenerator::VoxelTerrainGenerator(Config config) 
    : _done(false), _config(config), _gen_terrain_promises(512)
    {
    try {
        const auto threads_count = std::thread::hardware_concurrency();
        for (std::uint32_t i{ 0U }; i < threads_count; ++i) {
            _threads.push_back(std::jthread(&VoxelTerrainGenerator::thread, this));
        }
    } catch([[maybe_unused]] const std::exception&) {
        _done = true;
    }
}

void VoxelTerrainGenerator::thread() {
    _current_buffer = 0U;
    _tmp_noise.resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0.F);
    _noise_double_buffer[0].resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0U);
    _noise_double_buffer[1].resize(_config.terrain_size[0] * _config.terrain_size[1] * _config.terrain_size[2], 0U);
    // if (_config.noise_type == Config::NoiseType::SIMPLEX) {
        auto fn_simplex = FastNoise::New<FastNoise::Simplex>(FastSIMD::Level_AVX512);
        auto fn_fractal = FastNoise::New<FastNoise::FractalFBm>(FastSIMD::Level_AVX512);
        fn_fractal->SetSource(fn_simplex);
        fn_fractal->SetGain(11.F);
        fn_fractal->SetWeightedStrength(18.44F);
        fn_fractal->SetOctaveCount(2);
        fn_fractal->SetLacunarity(-.2F);

        auto fn_position_output = FastNoise::New<FastNoise::PositionOutput>(FastSIMD::Level_AVX512);
        fn_position_output->Set<FastNoise::Dim::Y>(-2.56F, -.14F);
        fn_position_output->Set<FastNoise::Dim::X>(-.42F, 0.F);

        auto fn_add = FastNoise::New<FastNoise::Add>(FastSIMD::Level_AVX512);
        fn_add->SetLHS(fn_fractal);
        fn_add->SetRHS(fn_position_output);
        _smart_node = fn_add;

    // } else {
    //     _smart_node = FastNoise::New<FastNoise::Perlin>(FastSIMD::Level_AVX512);
    // }

    while (!_done) {
        Promise promise;
        if (_gen_terrain_promises.read(promise)) {
            const auto p0 = Vec3i32::mul(promise.position, _config.terrain_size);
            const auto p1 = _config.terrain_size; //Vec3i32::add(p0, _config.terrain_size);

            _smart_node->GenUniformGrid3D(
                _tmp_noise.data(), 
                p0[0], p0[1], p0[2], p1[0], p1[1], p1[2], 
                _config.noise_frequency, _config.seed
            );

            auto& buffer = _noise_double_buffer[_current_buffer];

            std::size_t i{ 0UL };
            u32 not_empty{ 0UL };
            for (const auto noise_value : _tmp_noise) {
                const auto value = static_cast<u16>(std::clamp(std::roundf(((noise_value + 1.F)/2.F) * static_cast<f32>(_config.quantize_values)), 0.F, 1.F));
                not_empty += static_cast<u32>(value > 0);
                buffer[i++] = value;
            }
            if (not_empty) {
                _current_buffer = _current_buffer ^ 0x1U;
            }

            promise.gen_result_promise.set_value(not_empty ? std::optional(std::span<const u16>(buffer)) : std::nullopt);
        } else {
            std::this_thread::yield();
        }
    }
}

std::future<std::optional<std::span<const vmath::u16>>> VoxelTerrainGenerator::gen(Vec3i32 chunk_position) {
    std::promise<std::optional<std::span<const vmath::u16>>> promise;
    std::future<std::optional<std::span<const vmath::u16>>> result(promise.get_future());

    _gen_terrain_promises.write({std::move(promise), chunk_position});

    return result;
}