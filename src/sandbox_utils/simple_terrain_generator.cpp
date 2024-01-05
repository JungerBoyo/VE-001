#include "simple_terrain_generator.h"

using namespace ve001;
using namespace vmath;

thread_local std::vector<u16> SimpleTerrainGenerator::_data;

static void makeCorridor(std::vector<u16>& data, Vec3i32 chunk_size) {
    i32 i{ 0 };
    for(i32 z{ 0 }; z < chunk_size[2]; ++z) {
        for(i32 y{ 0 }; y < chunk_size[1]; ++y) {
            for(i32 x{ 0 }; x < chunk_size[0]; ++x) {
                if (((x >= 0 && x < chunk_size[0]/4) ||
                     (y >= 0 && y < chunk_size[1]/4) ||
                     (z >= 0 && z < chunk_size[2]/4)) 
                                && 
                    ((x >= 3*chunk_size[0]/4 && x < chunk_size[0]) ||
                     (y >= 3*chunk_size[1]/4 && y < chunk_size[1]) ||
                     (z >= 3*chunk_size[2]/4 && z < chunk_size[2]))
                ) {
                    data[i] = 1U;
                } else {
                    data[i] = 0U;
                }
                ++i;
            }
        }
    }
}

SimpleTerrainGenerator::SimpleTerrainGenerator(Vec3i32 chunk_size) : _chunk_size(chunk_size) {}

void SimpleTerrainGenerator::threadInit() {
    _data.resize(_chunk_size[0] *  _chunk_size[1] *  _chunk_size[2], 0U);
    makeCorridor(_data, _chunk_size);
}
std::optional<std::span<const u16>> SimpleTerrainGenerator::gen([[maybe_unused]]Vec3i32 chunk_position) {
    return std::span<const u16>(_data);
}
