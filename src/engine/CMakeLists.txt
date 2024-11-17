add_library(ve001 STATIC
    chunk_data_streamer.cpp
    chunk_pool.cpp
    engine.cpp
    gpu_buffer.cpp
    meshing_engine_base.cpp
    meshing_engine_gpu.cpp
    cpu_mesher.cpp
    meshing_engine_cpu.cpp
    shader.cpp
    world_grid.cpp
)

if (DEFINED ENGINE_TEST)
    target_compile_definitions(ve001 PUBLIC ENGINE_TEST)
endif()

target_compile_definitions(ve001 PRIVATE
    VE001_SH_CONFIG_ATTRIB_INDEX_POSITION=0
    VE001_SH_CONFIG_ATTRIB_INDEX_TEXCOORD=1
    VE001_SH_CONFIG_UBO_BINDING_MESHING_DESCRIPTOR=2
    VE001_SH_CONFIG_SSBO_BINDING_VOXEL_DATA=5
    VE001_SH_CONFIG_SSBO_BINDING_MESHING_TEMP=6
    VE001_SH_CONFIG_SSBO_BINDING_MESH_DATA=7
)

target_link_libraries(ve001 PRIVATE 
    glad::glad
    vmath
)
