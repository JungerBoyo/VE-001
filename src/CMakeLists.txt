find_package(fmt REQUIRED)
find_package(glad REQUIRED)
find_package(stb REQUIRED)
find_package(spdlog REQUIRED)
find_package(glm REQUIRED)

link_libraries(glm::glm)

if (USE_SDL2)
    add_compile_definitions(${PROJECT_NAME}_USE_SDL2=1)
    find_package(SDL2 REQUIRED)
elseif (USE_GLFW3)
    add_compile_definitions(${PROJECT_NAME}_USE_GLFW3=1)
    find_package(glfw3 REQUIRED)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(window)
add_subdirectory(sandbox_utils)
add_subdirectory(logger)
add_subdirectory(voxel_terrain_generator)
add_subdirectory(engine)

add_library(gl_context STATIC gl_context.h gl_context.cpp)
target_link_libraries(gl_context PUBLIC glad::glad)
if (USE_SDL2)
    target_link_libraries(gl_context PRIVATE SDL2::SDL2)
elseif (USE_GLFW3)
    target_link_libraries(gl_context PRIVATE glfw)
endif()

# if (DEBUG)
    target_link_libraries(gl_context PRIVATE logger)
# endif()

add_executable(${PROJECT_NAME} main.cpp)
add_executable(${PROJECT_NAME}_test main2.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    window 
    gl_context 
    vmath
    sandbox_utils 
    voxel_terrain_generator
    ve001_engine
    glfw
    sh_config
    logger
)
target_link_libraries(${PROJECT_NAME}_test PRIVATE 
    window 
    gl_context 
    vmath
    sandbox_utils 
    voxel_terrain_generator
    ve001_engine
    glfw
    sh_config
    logger
)
add_dependencies(${PROJECT_NAME} compile_and_copy_shaders_dirs)
add_dependencies(${PROJECT_NAME}_test compile_and_copy_shaders_dirs)

add_subdirectory(tools)




