#include "world_grid.h"

#include <iostream>

using namespace vmath;
using namespace ve001;

/*

*/

static bool isInElipsoid(Vec3f32 ellipsoid_center, Vec3f32 semi_axes, Vec3f32 point) {
    const auto diff = Vec3f32::sub(point, ellipsoid_center);
    return (
        (diff[0]*diff[0])/(semi_axes[0]*semi_axes[0]) +
        (diff[1]*diff[1])/(semi_axes[1]*semi_axes[1]) +
        (diff[2]*diff[2])/(semi_axes[2]*semi_axes[2])) <= 1.F;
    // return semi_axes[0]*semi_axes[0] > 
    //         (point[0] - ellipsoid_center[0]) * (point[0] - ellipsoid_center[0]) + 
    //         (point[1] - ellipsoid_center[1]) * (point[1] - ellipsoid_center[1]) + 
    //         (point[2] - ellipsoid_center[2]) * (point[2] - ellipsoid_center[2]);

    return true;
}

static bool isInBoundingBox(Vec3i32 p0, Vec3i32 p1, Vec3i32 point) {
    return 
        (point[0] >= p0[0] && 
         point[1] >= p0[1] &&
         point[2] >= p0[2] &&
         point[0] <= p1[0] && 
         point[1] <= p1[1] &&
         point[2] <= p1[2]);
}

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


WorldGrid::WorldGrid(const EngineContext& engine_context, Vec3f32 world_size, Vec3f32 initial_position) : _engine_context(engine_context), 
    _current_position(initial_position), _semi_axes(world_size), _chunk_pool(engine_context), 
    _grid_size(Vec3i32::add(Vec3i32::mulScalar(Vec3i32::cast(Vec3f32::div(world_size, Vec3f32::cast(engine_context.chunk_size))), 2), 1)),
    _terrain_generator({
        .terrain_size = _engine_context.chunk_size,
        .terrain_density = 16U,
        .noise_type = VoxelTerrainGenerator::Config::NoiseType::PERLIN,
        .quantize_values = 1U,
        .quantized_value_size = sizeof(u16),
        .seed = 0xABABDEAD
    }),
    _noise(_engine_context.chunk_size_1D, 1U) {
}

constexpr std::array<Vec3i32, 6> NEIGHBOURS_OFFSETS{{
    {1, 0, 0}, {-1, 0, 0},
    {0, 1, 0}, { 0,-1, 0},
    {0, 0, 1}, { 0, 0,-1},
}};

// #define DEBUG_WORLD_GRID

void WorldGrid::init() {
    // determine position in chunk space 
    // _grid_size = Vec3i32::sub(_grid_size, {1});
    const auto position_in_chunk_space = Vec3i32::cast(vmath::vroundf(Vec3f32::div(_current_position, Vec3f32::cast(_engine_context.chunk_size))));
    const auto half_grid_size = Vec3i32::divScalar(_grid_size, 2);//1;
    const auto world_p0_in_chunk_space = Vec3i32::sub(position_in_chunk_space, half_grid_size);
    const auto world_p1_in_chunk_space = Vec3i32::add(position_in_chunk_space, half_grid_size/*Vec3i32::sub(half_grid_size, {1})*/);

    _visible_chunks.reserve(_grid_size[0] * _grid_size[1] * _grid_size[2]);
    _tmp_indices.resize(_grid_size[0] * _grid_size[1] * _grid_size[2], VisibleChunk::INVALID_NEIGHBOUR_INDEX);

    u32 i{ 0U };
    for(i32 z{ world_p0_in_chunk_space[2] }; z <= world_p1_in_chunk_space[2]; ++z) {
        for(i32 y{ world_p0_in_chunk_space[1] }; y <= world_p1_in_chunk_space[1]; ++y) {
            for(i32 x{ world_p0_in_chunk_space[0] }; x <= world_p1_in_chunk_space[0]; ++x) {
                const Vec3i32 xyz(x, y, z);
                const auto xyz_real = Vec3f32::cast(Vec3i32::mul(_engine_context.chunk_size, xyz));
                if (isInElipsoid(_current_position, _semi_axes, xyz_real)) {
                    _visible_chunks.emplace_back(INVALID_CHUNK_ID, xyz);
                    const auto position_index = 
                        (x - world_p0_in_chunk_space[0]) +
                        (y - world_p0_in_chunk_space[1]) * _grid_size[0] +
                        (z - world_p0_in_chunk_space[2]) * _grid_size[0] * _grid_size[1];
                    _tmp_indices[position_index] = i++;
                }
            }
        }
    }
    _chunk_pool.init(static_cast<i32>(_visible_chunks.size()));

    makeCorridor(_noise, _engine_context.chunk_size);

    for(auto& chunk : _visible_chunks) {
        // _terrain_generator.next(static_cast<void*>(_noise.data()), 0U, 0U, chunk.position_in_chunks);
#ifdef DEBUG_WORLD_GRID
        chunk.chunk_id = INVALID_CHUNK_ID;
#else
        chunk.chunk_id = _chunk_pool.allocateChunk(std::span<const u16>(_noise), chunk.position_in_chunks);
#endif
        u32 neighbour{ 0U };
        for (const auto neighbour_offset : NEIGHBOURS_OFFSETS) {
            const auto neighbour_position = Vec3i32::add(chunk.position_in_chunks, neighbour_offset);
            if (isInBoundingBox(world_p0_in_chunk_space, world_p1_in_chunk_space, neighbour_position)) {
                const auto neighbour_position_index = 
                    (neighbour_position[0] - world_p0_in_chunk_space[0]) +
                    (neighbour_position[1] - world_p0_in_chunk_space[1]) * _grid_size[0] +
                    (neighbour_position[2] - world_p0_in_chunk_space[2]) * _grid_size[0] * _grid_size[1];
                if (const auto index = _tmp_indices[neighbour_position_index]; index != VisibleChunk::INVALID_NEIGHBOUR_INDEX) {
                   ++chunk.neighbours_count;
                   chunk.neighbours_indices[neighbour] = index;
                }
            }
            ++neighbour;
        }
    }
    std::fill(_tmp_indices.begin(), _tmp_indices.end(), VisibleChunk::INVALID_NEIGHBOUR_INDEX);
}

static u32 oppositeNeighbour(u32 neighbour) {
    return neighbour - (neighbour & 0x1U) + (~neighbour & 0x1U);
}

bool WorldGrid::validate() {
    const auto position_in_chunk_space = Vec3i32::cast(vmath::vroundf(Vec3f32::div(_current_position, Vec3f32::cast(_engine_context.chunk_size))));
    const auto half_grid_size = Vec3i32::divScalar(_grid_size, 2);//1;
    const auto world_p0_in_chunk_space = Vec3i32::sub(position_in_chunk_space, half_grid_size);
    const auto world_p1_in_chunk_space = Vec3i32::add(position_in_chunk_space, half_grid_size/*Vec3i32::sub(half_grid_size, {1})*/);

    for (const auto& visible_chunk : _visible_chunks) {
        if (isInBoundingBox(world_p0_in_chunk_space, world_p1_in_chunk_space, visible_chunk.position_in_chunks)) {
            u32 nonempty_neighbours{ 0U };
            for (u32 neighbour{ 0U }; neighbour < 6U; ++neighbour) {
                if (visible_chunk.neighbours_indices[neighbour] != VisibleChunk::INVALID_NEIGHBOUR_INDEX) {
                    const auto offset = NEIGHBOURS_OFFSETS[neighbour];
                    const auto& visible_chunk_neighbour = _visible_chunks[visible_chunk.neighbours_indices[neighbour]];

                    const auto comp_offset = Vec3i32::sub(visible_chunk_neighbour.position_in_chunks, visible_chunk.position_in_chunks);
                    ++nonempty_neighbours;
                    if (comp_offset[0] != offset[0] || 
                        comp_offset[1] != offset[1] ||
                        comp_offset[2] != offset[2]) {
                    
                        return false;                        
                    }
                }
            }
            if (visible_chunk.neighbours_count != nonempty_neighbours) {
                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}


void WorldGrid::update(Vec3f32 new_position) {
    static constexpr f32 epsilon{ .00001F };
    const auto move_vec = Vec3f32::sub(new_position, _current_position);
    if (std::abs(move_vec[0]) <= epsilon &&
        std::abs(move_vec[1]) <= epsilon &&
        std::abs(move_vec[2]) <= epsilon) {
        return;
    }

    // if (new_position[0] == _current_position[0] &&
    //     new_position[1] == _current_position[1] &&
    //     new_position[2] == _current_position[2]) {
        
    //     return;
    // }

    // // if (std::abs(move_vec[0]) > static_cast<f32>(_engine_context.chunk_size[0]) ||
    // //     std::abs(move_vec[1]) > static_cast<f32>(_engine_context.chunk_size[1]) ||
    // //     std::abs(move_vec[2]) > static_cast<f32>(_engine_context.chunk_size[2])) {
    // //     /// ...            
    // // }

    // if (std::abs(_current_position[0] - new_position[0]) <= static_cast<f32>(_engine_context.chunk_size[0])  &&
    //     std::abs(_current_position[1] - new_position[1]) <= static_cast<f32>(_engine_context.chunk_size[1])  && 
    //     std::abs(_current_position[2] - new_position[2]) <= static_cast<f32>(_engine_context.chunk_size[2])) {
    //     return;
    // }

    _current_position = new_position;

    // elipsa przesówa się a bounding box się nie przesówa bo jest dyskretny?? możliwe

    const auto position_in_chunk_space = Vec3i32::cast(vmath::vroundf(Vec3f32::div(_current_position, Vec3f32::cast(_engine_context.chunk_size))));
    const auto half_grid_size = Vec3i32::divScalar(_grid_size, 2);//1;
    const auto world_p0_in_chunk_space = Vec3i32::sub(position_in_chunk_space, half_grid_size);
    const auto world_p1_in_chunk_space = Vec3i32::add(position_in_chunk_space, half_grid_size/*Vec3i32::sub(half_grid_size, {1})*/);
    // const auto world_p1_in_chunk_space = Vec3i32::add(position_in_chunk_space, half_grid_size);


    auto start_visible_chunks_size = _visible_chunks.size();
    for (u32 i{ 0U }; i < start_visible_chunks_size;) {
        if (_visible_chunks[i].neighbours_count < 6) {
            // if (isInBoundingBox(world_p0_in_chunk_space, world_p1_in_chunk_space, _visible_chunks[i].position_in_chunks)) {
            //     const auto position = Vec3f32::cast(Vec3i32::mul(_engine_context.chunk_size, _visible_chunks[i].position_in_chunks));
            //     if (isInElipsoid(_current_position, _semi_axes, position)) {
                    for (u32 neighbour{ 0U }; neighbour < 6U; ++neighbour)  {
                        const auto neighbour_index = _visible_chunks[i].neighbours_indices[neighbour];
                        if (neighbour_index == VisibleChunk::INVALID_NEIGHBOUR_INDEX) {
                            const auto neighbour_position_in_chunks = Vec3i32::add(_visible_chunks[i].position_in_chunks, NEIGHBOURS_OFFSETS[neighbour]);
                            if (isInBoundingBox(world_p0_in_chunk_space, world_p1_in_chunk_space, neighbour_position_in_chunks)) {
                                const auto neighbour_position_index = 
                                    (neighbour_position_in_chunks[0] - world_p0_in_chunk_space[0]) +
                                    (neighbour_position_in_chunks[1] - world_p0_in_chunk_space[1]) * _grid_size[0] +
                                    (neighbour_position_in_chunks[2] - world_p0_in_chunk_space[2]) * _grid_size[0] * _grid_size[1];
                                if (_tmp_indices[neighbour_position_index] != VisibleChunk::INVALID_NEIGHBOUR_INDEX) {
                                    const auto neighbour_index = _tmp_indices[neighbour_position_index];
                                    _visible_chunks[i].neighbours_indices[neighbour] = neighbour_index;
                                    ++_visible_chunks[i].neighbours_count;
                                    _visible_chunks[neighbour_index].neighbours_indices[oppositeNeighbour(neighbour)] = i;
                                    ++_visible_chunks[neighbour_index].neighbours_count;
                                    continue;
                                }
                                const auto neighbour_position = Vec3f32::cast(Vec3i32::mul(neighbour_position_in_chunks, _engine_context.chunk_size));
                                if (!isInElipsoid(_current_position, _semi_axes, neighbour_position)) {
                                    continue;
                                }
                                const auto visible_neighbour_chunk_index = _visible_chunks.size();

                                std::cout << "hello { " << neighbour_position_in_chunks[0] << " " 
                                                        << neighbour_position_in_chunks[1] << " "
                                                        << neighbour_position_in_chunks[2] << " }\n";
                                if (
                                    neighbour_position_in_chunks[0] == 0 &&
                                    neighbour_position_in_chunks[1] == 1 &&
                                    neighbour_position_in_chunks[2] == 1                                
                                ) {
                                    std::cout << "here!!\n";
                                }

#ifndef DEBUG_WORLD_GRID
                                const auto chunk_id = _chunk_pool.allocateChunk(std::span<const u16>(_noise), neighbour_position_in_chunks);
                                if (chunk_id == INVALID_CHUNK_ID) {
                                    _to_allocate_chunks.push_back(neighbour_position_in_chunks);                                     
                                }
                                auto& visible_neighbour_chunk = _visible_chunks.emplace_back(chunk_id, neighbour_position_in_chunks);
#else
                                auto& visible_neighbour_chunk = _visible_chunks.emplace_back(INVALID_CHUNK_ID, neighbour_position_in_chunks);
#endif
                                _visible_chunks[i].neighbours_indices[neighbour] = visible_neighbour_chunk_index;
                                ++_visible_chunks[i].neighbours_count;
                                visible_neighbour_chunk.neighbours_indices[oppositeNeighbour(neighbour)] = i;
                                ++visible_neighbour_chunk.neighbours_count;
                                _tmp_indices[neighbour_position_index] = visible_neighbour_chunk_index;

                                for (u32 neighbour_neighbour{ 0U }; neighbour_neighbour < 6U; ++neighbour_neighbour) {
                                    const auto neighbour_neighbour_position_in_chunks = Vec3i32::add(neighbour_position_in_chunks, NEIGHBOURS_OFFSETS[neighbour_neighbour]);
                                    if (isInBoundingBox(world_p0_in_chunk_space, world_p1_in_chunk_space, neighbour_neighbour_position_in_chunks)) {
                                        const auto nieghbour_neighbour_position_index = 
                                            (neighbour_neighbour_position_in_chunks[0] - world_p0_in_chunk_space[0]) +
                                            (neighbour_neighbour_position_in_chunks[1] - world_p0_in_chunk_space[1]) * _grid_size[0] +
                                            (neighbour_neighbour_position_in_chunks[2] - world_p0_in_chunk_space[2]) * _grid_size[0] * _grid_size[1];
                                        
                                        if (_tmp_indices[nieghbour_neighbour_position_index] != VisibleChunk::INVALID_NEIGHBOUR_INDEX) {
                                            const auto visible_neighbour_neighbour_chunk_index = _tmp_indices[nieghbour_neighbour_position_index];
                                            auto& visible_neighbour_neighbour_chunk = _visible_chunks[visible_neighbour_neighbour_chunk_index];

                                            visible_neighbour_chunk.neighbours_indices[neighbour_neighbour] = visible_neighbour_neighbour_chunk_index;
                                            ++visible_neighbour_chunk.neighbours_count;
                                            visible_neighbour_neighbour_chunk.neighbours_indices[oppositeNeighbour(neighbour_neighbour)] = visible_neighbour_chunk_index;
                                            ++visible_neighbour_neighbour_chunk.neighbours_count;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    // ++i;
                    // continue;
            //     }
            // }
            
            const auto position = Vec3f32::cast(Vec3i32::mul(_engine_context.chunk_size, _visible_chunks[i].position_in_chunks));
            if (!isInBoundingBox(world_p0_in_chunk_space, world_p1_in_chunk_space, _visible_chunks[i].position_in_chunks) ||
                !isInElipsoid(_current_position, _semi_axes, position)) {

                const auto increment = static_cast<u32>(_visible_chunks.size() > start_visible_chunks_size);

                const auto removed_chunk_id = _visible_chunks[i].chunk_id;

                const auto last_chunk_index = static_cast<u32>(_visible_chunks.size() - 1U);
                if (i != last_chunk_index) {
                    const auto last_chunk = _visible_chunks.back();
                    const auto last_chunk_position_index = 
                        (last_chunk.position_in_chunks[0] - world_p0_in_chunk_space[0]) +
                        (last_chunk.position_in_chunks[1] - world_p0_in_chunk_space[1]) * _grid_size[0] +
                        (last_chunk.position_in_chunks[2] - world_p0_in_chunk_space[2]) * _grid_size[0] * _grid_size[1];

                    if (_tmp_indices[last_chunk_position_index] != VisibleChunk::INVALID_NEIGHBOUR_INDEX) {
                        _tmp_indices[last_chunk_position_index] = i;
                    }

                    for (u32 neighbour{ 0U }; neighbour < 6; ++neighbour) {
                        const auto neighbour_index = _visible_chunks[i].neighbours_indices[neighbour];
                        if (neighbour_index != VisibleChunk::INVALID_NEIGHBOUR_INDEX) {
                            auto& neighbour_chunk = _visible_chunks[neighbour_index];
                            neighbour_chunk.neighbours_indices[oppositeNeighbour(neighbour)] = VisibleChunk::INVALID_NEIGHBOUR_INDEX;
                            --neighbour_chunk.neighbours_count;
                        }
                    }
                    
                    for (u32 neighbour{ 0U }; neighbour < 6; ++neighbour) {
                        const auto neighbour_index = last_chunk.neighbours_indices[neighbour];
                        if (neighbour_index != VisibleChunk::INVALID_NEIGHBOUR_INDEX) {
                            _visible_chunks[neighbour_index].neighbours_indices[oppositeNeighbour(neighbour)] = i;
                        }
                    }
                    
                    _visible_chunks[i] = _visible_chunks.back();
                } else {
                    for (u32 neighbour{ 0U }; neighbour < 6; ++neighbour) {
                        const auto neighbour_index = _visible_chunks[i].neighbours_indices[neighbour];
                        if (neighbour_index != VisibleChunk::INVALID_NEIGHBOUR_INDEX) {
                            auto& neighbour_chunk = _visible_chunks[neighbour_index];
                            neighbour_chunk.neighbours_indices[oppositeNeighbour(neighbour)] = VisibleChunk::INVALID_NEIGHBOUR_INDEX;
                            --neighbour_chunk.neighbours_count;
                        }
                    }
                }

                std::cout << "goodbye { " << position[0] << ", " << position[1] << ", " << position[2] << " }\n";
                _visible_chunks.pop_back();
                start_visible_chunks_size -= (1-increment);
                i += increment;
#ifndef DEBUG_WORLD_GRID
                _chunk_pool.deallocateChunk(removed_chunk_id);
#endif
            } else {
                ++i;
            }
        } else {
            ++i;
        }
    }

    for (const auto to_allocate_chunk : _to_allocate_chunks) {
        const auto to_allocate_chunk_index = 
            (to_allocate_chunk[0] - world_p0_in_chunk_space[0]) +
            (to_allocate_chunk[1] - world_p0_in_chunk_space[1]) * _grid_size[0] +
            (to_allocate_chunk[2] - world_p0_in_chunk_space[2]) * _grid_size[0] * _grid_size[1];
        _visible_chunks[_tmp_indices[to_allocate_chunk_index]].chunk_id = _chunk_pool.allocateChunk(std::span<const u16>(_noise), to_allocate_chunk);
    }
    _to_allocate_chunks.clear();

    std::cout << "new position = { " << position_in_chunk_space[0] << " " << position_in_chunk_space[1] << " " << position_in_chunk_space[2] << " }\n";
    if(!validate()) {
        std::cout << "validation failed!\n";
    }
    

    std::fill(_tmp_indices.begin(), _tmp_indices.end(), VisibleChunk::INVALID_NEIGHBOUR_INDEX);
}

void WorldGrid::deinit() {
    _chunk_pool.deinit();
}