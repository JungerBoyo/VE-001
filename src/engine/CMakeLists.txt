add_library(ve001_engine STATIC
    chunk_mesh_pool.h
    chunk_mesh_pool.cpp
    errors.h
    ringbuffer.h
)
target_link_libraries(ve001_engine PRIVATE glad::glad)