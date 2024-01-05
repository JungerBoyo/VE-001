#include "world_grid.h"

using namespace vmath;
using namespace ve001;

static bool isInElipsoid(Vec3f32 ellipsoid_center, Vec3f32 semi_axes, Vec3f32 point) {
    const auto diff = Vec3f32::sub(point, ellipsoid_center);
    return (
        (diff[0]*diff[0])/(semi_axes[0]*semi_axes[0]) +
        (diff[1]*diff[1])/(semi_axes[1]*semi_axes[1]) +
        (diff[2]*diff[2])/(semi_axes[2]*semi_axes[2])) <= 1.F;
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

static u32 computeMaxVisibleChunks(Vec3i32 chunk_size, Vec3f32 ellipsoid_semi_axes) {
    const auto chunk_size_f32 = Vec3f32::cast(chunk_size);
    const auto max_chunks_along_x = static_cast<i32>(std::floor((2.F * ellipsoid_semi_axes[0])/chunk_size_f32[0]));
    const auto max_chunks_along_y = static_cast<i32>(std::floor((2.F * ellipsoid_semi_axes[1])/chunk_size_f32[1]));
    const auto max_chunks_along_z = static_cast<i32>(std::floor((2.F * ellipsoid_semi_axes[2])/chunk_size_f32[2]));

    return max_chunks_along_x * max_chunks_along_y * max_chunks_along_z;
}

WorldGrid::WorldGrid(
    EngineContext& engine_context, 
    Vec3f32 world_size, 
    Vec3f32 initial_position, 
    u32 chunk_data_streamer_threads_count,
    std::unique_ptr<ChunkGenerator> chunk_generator) :

    _engine_context(engine_context),
    _max_visible_chunks(computeMaxVisibleChunks(_engine_context.chunk_size, world_size)),
    _current_position(initial_position), _semi_axes(world_size), _chunk_pool(engine_context, _max_visible_chunks), 
    _grid_size(Vec3i32::add(Vec3i32::mulScalar(Vec3i32::cast(Vec3f32::div(world_size, Vec3f32::cast(engine_context.chunk_size))), 2), 1)),
    _chunk_data_streamer(chunk_data_streamer_threads_count, std::move(chunk_generator), _max_visible_chunks),
    _to_allocate_chunks(_max_visible_chunks) {

    if (_chunk_data_streamer._done) {
        _engine_context.error |= Error::CHUNK_DATA_STREAMER_THREAD_ALLOCATION_FAILED;
    }
}

constexpr std::array<Vec3i32, 6> NEIGHBOURS_OFFSETS{{
    {1, 0, 0}, {-1, 0, 0},
    {0, 1, 0}, { 0,-1, 0},
    {0, 0, 1}, { 0, 0,-1},
}};

void WorldGrid::init() {
    if (_engine_context.error != Error::NO_ERROR) {
        return;
    }

    const auto chunk_size_f32 = Vec3f32::cast(_engine_context.chunk_size);
    const auto max_chunks_along_x = static_cast<i32>(std::floor((2.F * _semi_axes[0])/chunk_size_f32[0]));
    const auto max_chunks_along_y = static_cast<i32>(std::floor((2.F * _semi_axes[1])/chunk_size_f32[1]));
    const auto max_chunks_along_z = static_cast<i32>(std::floor((2.F * _semi_axes[2])/chunk_size_f32[2]));
    const auto max_chunks = max_chunks_along_x*max_chunks_along_y*max_chunks_along_z;
   
    _chunk_pool.init();

    try {
        _tmp_indices.resize(_grid_size[0] * _grid_size[1] * _grid_size[2], VisibleChunk::INVALID_NEIGHBOUR_INDEX);
        _visible_chunks.reserve(max_chunks);
        _free_visible_chunk_ids.resize(max_chunks);
        _visible_chunk_id_to_index.resize(max_chunks, INVALID_VISIBLE_CHUNK_INDEX);
    } catch ([[maybe_unused]] const std::exception&) {
        _engine_context.error |= Error::CPU_ALLOCATION_FAILED;
        return;
    }

    u32 i{ 0U };
    for (auto& visible_chunk_id : _free_visible_chunk_ids) {
        visible_chunk_id = i++;
    }

    const auto position_in_chunk_space = Vec3i32::cast(vmath::vroundf(Vec3f32::div(_current_position, Vec3f32::cast(_engine_context.chunk_size))));
    const auto half_grid_size = Vec3i32::divScalar(_grid_size, 2);//1;
    const auto world_p0_in_chunk_space = Vec3i32::sub(position_in_chunk_space, half_grid_size);
    const auto world_p1_in_chunk_space = Vec3i32::add(position_in_chunk_space, half_grid_size);
    i = 0U; 
    for(i32 z{ world_p0_in_chunk_space[2] }; z <= world_p1_in_chunk_space[2]; ++z) {
        for(i32 y{ world_p0_in_chunk_space[1] }; y <= world_p1_in_chunk_space[1]; ++y) {
            for(i32 x{ world_p0_in_chunk_space[0] }; x <= world_p1_in_chunk_space[0]; ++x) {
                const Vec3i32 xyz(x, y, z);
                const auto xyz_real = Vec3f32::cast(Vec3i32::mul(_engine_context.chunk_size, xyz));
                if (isInElipsoid(_current_position, _semi_axes, xyz_real)) {
                    const auto visible_chunk_id = _free_visible_chunk_ids.back();
                    _free_visible_chunk_ids.pop_back();
                    _visible_chunk_id_to_index[visible_chunk_id] = static_cast<u32>(_visible_chunks.size());
                    _visible_chunks.emplace_back(visible_chunk_id, INVALID_CHUNK_ID, xyz);
                    const auto position_index = 
                        (x - world_p0_in_chunk_space[0]) +
                        (y - world_p0_in_chunk_space[1]) * _grid_size[0] +
                        (z - world_p0_in_chunk_space[2]) * _grid_size[0] * _grid_size[1];
                    _tmp_indices[position_index] = i++;
                }
            }
        }
    }

    for(auto& chunk : _visible_chunks) {
        _to_allocate_chunks.write({
            std::move(_chunk_data_streamer.gen(chunk.position_in_chunks)),
            chunk.visible_chunk_id
        });
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

        pollToAllocateChunks();
    }

    while (pollToAllocateChunks()) {}

    std::fill(_tmp_indices.begin(), _tmp_indices.end(), VisibleChunk::INVALID_NEIGHBOUR_INDEX);
}

static u32 oppositeNeighbour(u32 neighbour) {
    return neighbour - (neighbour & 0x1U) + (~neighbour & 0x1U);
}

void WorldGrid::update(Vec3f32 new_position) {
    static constexpr f32 epsilon{ .01F };
    const auto move_vec = Vec3f32::sub(new_position, _current_position);
    if (std::abs(move_vec[0]) <= epsilon &&
        std::abs(move_vec[1]) <= epsilon &&
        std::abs(move_vec[2]) <= epsilon) {
        return;
    }

    _current_position = new_position;

    const auto position_in_chunk_space = Vec3i32::cast(vmath::vroundf(Vec3f32::div(_current_position, Vec3f32::cast(_engine_context.chunk_size))));
    const auto half_grid_size = Vec3i32::divScalar(_grid_size, 2);
    const auto world_p0_in_chunk_space = Vec3i32::sub(position_in_chunk_space, half_grid_size);
    const auto world_p1_in_chunk_space = Vec3i32::add(position_in_chunk_space, half_grid_size);

    auto start_visible_chunks_size = _visible_chunks.size();
    for (u32 i{ 0U }; i < start_visible_chunks_size;) {
        if (_visible_chunks[i].neighbours_count < 6) {
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

                        const auto visible_chunk_id = _free_visible_chunk_ids.back();
                        _free_visible_chunk_ids.pop_back();
                        _visible_chunk_id_to_index[visible_chunk_id] = visible_neighbour_chunk_index;
                        auto& visible_neighbour_chunk = _visible_chunks.emplace_back(visible_chunk_id, INVALID_CHUNK_ID, neighbour_position_in_chunks);
                        _to_allocate_chunks.write({
                            std::move(_chunk_data_streamer.gen(neighbour_position_in_chunks)),
                            visible_chunk_id
                        });

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
            
            const auto position = Vec3f32::cast(Vec3i32::mul(_engine_context.chunk_size, _visible_chunks[i].position_in_chunks));
            if (!isInBoundingBox(world_p0_in_chunk_space, world_p1_in_chunk_space, _visible_chunks[i].position_in_chunks) ||
                !isInElipsoid(_current_position, _semi_axes, position)) {

                const auto increment = static_cast<u32>(_visible_chunks.size() > start_visible_chunks_size);

                const auto removed_chunk_id = _visible_chunks[i].chunk_id;

                const auto chunk = _visible_chunks[i];

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
                    
                    _visible_chunk_id_to_index[last_chunk.visible_chunk_id] = i;
                    
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

                _free_visible_chunk_ids.push_back(chunk.visible_chunk_id);
                _visible_chunk_id_to_index[chunk.visible_chunk_id] = INVALID_VISIBLE_CHUNK_INDEX;
                _visible_chunks.pop_back();
                start_visible_chunks_size -= (1-increment);
                i += increment;
                _chunk_pool.deallocateChunk(removed_chunk_id);
            } else {
                ++i;
            }
            
            pollToAllocateChunks();
        } else {
            ++i;
        }
    }

    while (pollToAllocateChunks()) {}

    std::fill(_tmp_indices.begin(), _tmp_indices.end(), VisibleChunk::INVALID_NEIGHBOUR_INDEX);
}

bool WorldGrid::pollToAllocateChunks() {
    if (ToAllocateChunk* to_allocate_chunk{ nullptr }; _to_allocate_chunks.peek(to_allocate_chunk) && to_allocate_chunk != nullptr) {
        if (to_allocate_chunk->data.valid()) {
            if (to_allocate_chunk->data.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                if (const auto visible_chunk_index = _visible_chunk_id_to_index[to_allocate_chunk->visible_chunk_id]; visible_chunk_index != INVALID_VISIBLE_CHUNK_INDEX) {
                    const auto data = to_allocate_chunk->data.get();
                    if (data.has_value()) {
                        const auto chunk_id = _chunk_pool.allocateChunk(data.value(), _visible_chunks[visible_chunk_index].position_in_chunks);
                        if (chunk_id != INVALID_CHUNK_ID) {
                            _visible_chunks[visible_chunk_index].chunk_id = chunk_id;
                        } else {
                            to_allocate_chunk->ready_data = data.value();
                        }
                    }
                    _to_allocate_chunks.emptyRead();
                } else {
                    _to_allocate_chunks.emptyRead();
                }
            }
        } else {
            if (const auto visible_chunk_index = _visible_chunk_id_to_index[to_allocate_chunk->visible_chunk_id]; visible_chunk_index != INVALID_VISIBLE_CHUNK_INDEX) {
                if (to_allocate_chunk->ready_data.has_value()) {
                    const auto chunk_id = _chunk_pool.allocateChunk(to_allocate_chunk->ready_data.value(), _visible_chunks[visible_chunk_index].position_in_chunks);
                    if (chunk_id != INVALID_CHUNK_ID) {
                        _visible_chunks[visible_chunk_index].chunk_id = chunk_id;
                    } 
                } else {
                    _to_allocate_chunks.emptyRead();
                }
            } else {
                to_allocate_chunk->ready_data = std::nullopt;
                _to_allocate_chunks.emptyRead();
            }
        }
        return true;
    } 
    return false;
}

void WorldGrid::deinit() {
    _chunk_pool.deinit();
}