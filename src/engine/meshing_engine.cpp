#include "meshing_engine.h"

using namespace vmath;
using namespace ve001;

MeshingEngine::MeshingEngine(Config config) : config(config) {}

std::array<u32, 6> MeshingEngine::mesh(
    void* dst, 
    u32 offset, 
    u32 stride, 
    u32 face_stride, 
    Vec3f32 position, 
    const std::function<bool(i32, i32, i32)>& fn_visibility_query,
    const std::function<void(void*, MeshedRegionDescriptor)>& fn_write_quad
) {

    std::array<u32, 6> result;
    for (u32 i{ 0U }; i < 6U; ++i) {
        result[i] = meshAxis(
            static_cast<MeshingFace>(i), 
            static_cast<void*>(static_cast<u8*>(dst) + i * face_stride), 
            face_stride, 
            offset, 
            stride, 
            position, 
            fn_visibility_query,
            fn_write_quad
        );    
    }
    return result;
}

u32 MeshingEngine::meshAxis(
    MeshingFace meshing_face, 
    void* face_dst, 
    u32 face_dst_max_size,
    u32 offset, 
    u32 stride, 
    Vec3f32 position, 
    const std::function<bool(i32, i32, i32)>& fn_visibility_query,
    const std::function<void(void*, MeshedRegionDescriptor)>& fn_write_quad
) {
    u32 result{ 0U };
    // axis id X, Y or Z
    const u32 axis_id = meshing_face / 2;

    const Vec3u32 logical_indices((axis_id + 1) % 3, (axis_id + 2) % 3, axis_id);
    const Vec3u32 real_indices((2U + axis_id * 2U) % 3, (0U + axis_id * 2U) % 3, (1U + axis_id * 2U) % 3);

    // positions the args in logical way where 
    // Z - is the current axis
    // X, Y - are extents of the plane perpedicular to the axis
    const Vec3i32 logical_extent(
        config.chunk_size[logical_indices[0]],
        config.chunk_size[logical_indices[1]],
        config.chunk_size[logical_indices[2]]
    );
    
    u8* face_dst_u8 = static_cast<u8*>(face_dst) + offset;

    // dynamic bitset
    std::vector<bool> plane_of_states(logical_extent[0]*logical_extent[1], false);

    // depending on the meshing axis (eg. pos/neg), 
    const i32 edge_value = (meshing_face % 2) == 1 ? logical_extent[2] : 0;
    const i32 polarity = (meshing_face % 2) == 1 ? 1 : -1;
    // axis iteration loop(plane/slice-wise)
    Vec3i32 i(0);
    for (; i[2] < logical_extent[2]; ++i[2]) {
        for (; i[1] < logical_extent[1]; ++i[1]) {
            for (; i[0] < logical_extent[0]; ++i[0]) {
                if (fn_visibility_query(i[real_indices[0]], i[real_indices[1]], i[real_indices[2]])) {
                    if (i[2] == edge_value) {
                        plane_of_states[i[0] + i[1] * logical_extent[0]] = true;
                        continue;
                    } 
                    i[2] += polarity;
                    if (!fn_visibility_query(i[real_indices[0]], i[real_indices[1]], i[real_indices[2]])) {
                        plane_of_states[i[0] + i[1] * logical_extent[0]] = true;
                    }
                    i[2] -= polarity;
                }
            }
        }

        i[0] = 0;
        i[1] = 0;
        for(; i[1] < logical_extent[1]; ++i[1]) {
            while(i[0] < logical_extent[0]) {
                if (plane_of_states[i[0] + i[1] * logical_extent[0]]) {
                    Vec2i32 mesh_region(1);
                    for ( ;i[0] + mesh_region[0] < logical_extent[0]; ++mesh_region[0]) {
                        if (!plane_of_states[(i[0] + mesh_region[0]) + i[1] * logical_extent[0]]) {
                           break; 
                        }
                    }

                    for ( ;i[1] + mesh_region[1] < logical_extent[1]; ++mesh_region[1]) {
                        for (i32 tmp_mesh_region{ 0 }; i[0] + tmp_mesh_region < i[0] + mesh_region[0]; ++tmp_mesh_region) {
                            if (!plane_of_states[(i[0] + mesh_region[0]) + i[1] * logical_extent[0]]) {
                                goto break_outer;
                            }
                        }
                    }
                break_outer:
                    Vec3i32 region_extent(0);
                    region_extent[logical_indices[2]] = 1;
                    region_extent[logical_indices[1]] = mesh_region[1];
                    region_extent[logical_indices[0]] = mesh_region[0];
                    
                    Vec2i32 squashed_region_extent(mesh_region[0], mesh_region[1]);                    

                    Vec3i32 region_offset(0);
                    region_offset[logical_indices[2]] = i[2];
                    region_offset[logical_indices[1]] = i[1];
                    region_offset[logical_indices[0]] = i[0];

                    fn_write_quad(face_dst, {
                        .face = meshing_face,
                        .region_extent = region_extent,
                        .region_offset = region_offset,
                        .squashed_region_extent = squashed_region_extent
                    });
                    result += 6;

                    face_dst_u8 += stride;
                    if (face_dst_u8 - static_cast<u8*>(face_dst) > face_dst_max_size) {
                        return result;
                    }

                    for (i32 y{ i[1] }; y < i[1] + mesh_region[1]; ++y) {
                        for (i32 x{ i[0] }; x < i[0] + mesh_region[0]; ++x) {
                            plane_of_states[x + y * logical_extent[0]] = false;
                        }
                    }

                    i[0] += mesh_region[0];
                } else {
                    ++i[0];
                }
            }
        }
        std::fill(plane_of_states.begin(), plane_of_states.end(), false);
    }

    return result;
}