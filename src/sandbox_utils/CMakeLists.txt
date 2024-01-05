if (NOT "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    add_library(sandbox_utils STATIC
        camera.h
        camera.cpp
        noise_terrain_generator.h
        noise_terrain_generator.cpp
        simple_terrain_generator.h
        simple_terrain_generator.cpp
    )
    target_link_libraries(sandbox_utils PUBLIC
        glad::glad
        ve001
        FastNoise2::FastNoise
        vmath
    )
else()
    add_library(sandbox_utils STATIC
        camera.h
        camera.cpp
        simple_terrain_generator.h
        simple_terrain_generator.cpp
    )
    target_link_libraries(sandbox_utils PUBLIC
        glad::glad
        ve001
        vmath
    )
endif()