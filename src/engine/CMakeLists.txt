add_library(ve001_engine STATIC
    chunk_mesh_pool.h
    chunk_mesh_pool.cpp
    errors.h
    ringbuffer.h
    meshing_engine.h
    meshing_engine.cpp
    cubemap.h
    cubemap.cpp
    enums.h
    texture_rgba8_array.h
    texture_rgba8_array.cpp
)
target_link_libraries(ve001_engine PRIVATE 
    glad::glad
    stb_impl
)