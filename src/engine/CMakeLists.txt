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
    texture_params.h
    texture_array.h
    texture_array.cpp
    texture.h
    texture.cpp
    shader.h
    shader.cpp
    gpu_dynamic_array.h
    gpu_dynamic_array.cpp
    material_allocator.h
    material_allocator.cpp
    framebuffer.h
    framebuffer.cpp
    camera.h
    camera.cpp
    light.h
    lighting.h
    lighting.cpp
    gpu_object_pooling_resource.h
    gpu_buffer.h
    gpu_buffer.cpp
    cubetexture_array.h
    cubetexture_array.cpp

    engine_context.h
    
    shader_repository.h
    shader_repository.cpp

    chunk_pool.h
    chunk_pool.cpp
    vertex.h

    meshing_engine2.h
    meshing_engine2.cpp

    engine.h
    engine.cpp

    world_grid.h
    world_grid.cpp
)
target_link_libraries(ve001_engine PRIVATE 
    glad::glad
    stb_impl
)