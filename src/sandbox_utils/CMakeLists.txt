add_library(sandbox_utils STATIC
    camera.h
    camera.cpp
    noise_terrain_generator.h
    noise_terrain_generator.cpp
    simple_terrain_generator.h
    simple_terrain_generator.cpp
)
target_link_libraries(sandbox_utils PRIVATE 
    glad::glad
    ve001
    FastNoise2::FastNoise
    vmath
)