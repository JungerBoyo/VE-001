add_library(voxel_terrain_generator STATIC
    voxel_terrain_generator.h
    voxel_terrain_generator.cpp
    noise_func.h
    noise_func.cpp
)
# target_link_libraries(voxel_terrain_generator PRIVATE 
#     vmath
# )