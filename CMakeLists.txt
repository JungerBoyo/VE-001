cmake_minimum_required(VERSION 3.20)

### before-project config ###

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

#############################


### project setup ###

project(VE001
    VERSION 1.0.0
    HOMEPAGE_URL "https://github.com/JungerBoyo/VE-001"
)

######################


### sub dirs ###

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/res
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_subdirectory(res)
add_subdirectory(shaders)
add_subdirectory(src)

################

### add conan install as dependency ###
### package project ###
install(TARGETS ve001-benchmark RUNTIME DESTINATION "./")
install(TARGETS ve001 DESTINATION "./engine/lib/")
install(TARGETS vmath DESTINATION "./vmath/lib/")
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/res/vmath/vmath.h
    ${CMAKE_CURRENT_SOURCE_DIR}/res/vmath/vmath_types.h

    DESTINATION "./vmath/include/")
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/chunk_data_streamer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/chunk_generator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/chunk_id.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/chunk_pool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/engine_context.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/enums.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/gpu_buffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/meshing_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/ringbuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/threadsafe_ringbuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/vertex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/world_grid.h

    DESTINATION "./engine/include/")

install(DIRECTORY ${PROJECT_SOURCE_DIR}/shaders/bin DESTINATION "./shaders/")
install(DIRECTORY ${PROJECT_SOURCE_DIR}/shaders/src DESTINATION "./shaders/")

include(CPack)
########################