add_library(ve001_engine STATIC
    errors.h
    ringbuffer.h
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

    chunk_id.h

    chunk_pool.h
    chunk_pool.cpp
    vertex.h

    meshing_engine.h
    meshing_engine.cpp

    engine.h
    engine.cpp

    world_grid.h
    world_grid.cpp

    # thread_pool.h
    # threadsafe_ringbuffer.h

    chunk_data_streamer.h
    chunk_data_streamer.cpp

    chunk_generator.h

    noise_terrain_generator.h
    noise_terrain_generator.cpp

    simple_terrain_generator.h
    simple_terrain_generator.cpp

    from_ve0_fmt_generator.h
    from_ve0_fmt_generator.cpp
)
target_link_libraries(ve001_engine PRIVATE 
    glad::glad
    voxel_terrain_generator
    stb_impl
    # FastNoise2::FastNoise
)