#ifndef VE001_SIMPLE_TERRAIN_GENERATOR_H
#define VE001_SIMPLE_TERRAIN_GENERATOR_H

#include "engine/chunk_generator.h"

#include <FastNoise/FastNoise.h>

#include <vector>

namespace ve001 {

struct SimpleTerrainGenerator : public ChunkGenerator {
    static thread_local std::vector<vmath::u16> _data;
    vmath::Vec3i32 _chunk_size;

    SimpleTerrainGenerator(vmath::Vec3i32 chunk_size);

    void threadInit() override;
    std::optional<std::span<const vmath::u16>> gen([[maybe_unused]] vmath::Vec3i32 chunk_position) override;
};

}

#endif