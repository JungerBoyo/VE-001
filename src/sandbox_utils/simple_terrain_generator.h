#ifndef VE001_SIMPLE_TERRAIN_GENERATOR_H
#define VE001_SIMPLE_TERRAIN_GENERATOR_H

#include "engine/chunk_generator.h"

#include <vector>

namespace ve001 {

struct SimpleTerrainGenerator : public ChunkGenerator {
    std::vector<vmath::u16> _noise;

    SimpleTerrainGenerator(vmath::Vec3i32 chunk_size);

    void threadInit() override;
    std::optional<std::span<const vmath::u16>> gen([[maybe_unused]] vmath::Vec3i32 chunk_position) override;
};

}

#endif