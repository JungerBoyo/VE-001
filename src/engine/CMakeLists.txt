add_library(ve001 STATIC
    chunk_data_streamer.cpp
    chunk_data_streamer.h
    chunk_generator.h
    chunk_id.h
    chunk_pool.cpp
    chunk_pool.h
    engine_context.h
    engine.cpp
    engine.h
    enums.h
    gpu_buffer.cpp
    gpu_buffer.h
    meshing_engine.cpp
    meshing_engine.h
    ringbuffer.h
    shader.cpp
    shader.h
    threadsafe_ringbuffer.h
    vertex.h
    world_grid.cpp
    world_grid.h
)
target_compile_definitions(ve001 PUBLIC ENGINE_TEST)
target_link_libraries(ve001 PRIVATE 
    glad::glad
    vmath
)