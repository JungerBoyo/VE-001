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

if (ENGINE_TEST)
    target_compile_definitions(ve001 PUBLIC ENGINE_TEST)
endif()
if (ENGINE_TEST_NONINTERACTIVE)
    target_compile_definitions(ve001 PUBLIC ENGINE_TEST_NONINTERACTIVE)
endif()
if (ENGINE_MEMORY_TEST)
    target_compile_definitions(ve001 PUBLIC ENGINE_MEMORY_TEST)
endif()
if (ENGINE_SHADERS_TEST)
    target_compile_definitions(ve001 PUBLIC ENGINE_SHADERS_TEST)
endif()
if (FORCE_USE_SHADER_FROM_SRC)
    target_compile_definitions(ve001 PUBLIC FORCE_USE_SHADER_FROM_SRC)
endif()
if (USE_VOLUME_TEXTURE_3D)
    target_compile_definitions(ve001 PUBLIC USE_VOLUME_TEXTURE_3D)
endif()

target_compile_definitions(ve001 PRIVATE
    VE001_SH_CONFIG_ATTRIB_INDEX_POSITION=0
    VE001_SH_CONFIG_ATTRIB_INDEX_TEXCOORD=1
    VE001_SH_CONFIG_UBO_BINDING_MESHING_DESCRIPTOR=2
    VE001_SH_CONFIG_SSBO_BINDING_VOXEL_DATA=5
    VE001_SH_CONFIG_SSBO_BINDING_MESHING_TEMP=6
    VE001_SH_CONFIG_SSBO_BINDING_MESH_DATA=7
	VE001_SH_CONFIG_SSBO_BINDING_TIMINGS_DATA=8
	VE001_SH_CONFIG_IMAGE_BINDING_VOLUME_3D=0
)

target_link_libraries(ve001 PRIVATE 
    glad::glad
    vmath
)
