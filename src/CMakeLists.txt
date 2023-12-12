find_package(glad REQUIRED)
find_package(spdlog REQUIRED)
find_package(FastNoise2 REQUIRED)
find_package(CLI11 REQUIRED)

link_libraries(FastNoise2::FastNoise)

find_package(glfw3 REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(window)
add_subdirectory(engine)
add_subdirectory(sandbox_utils)
add_subdirectory(logger)

add_library(gl_context STATIC gl_context.h gl_context.cpp)
target_link_libraries(gl_context PUBLIC glad::glad)
target_link_libraries(gl_context PRIVATE glfw)

target_link_libraries(gl_context PRIVATE logger)

add_executable(ve001-benchmark ve001_benchmark.cpp testing_context.h)

target_link_libraries(ve001-benchmark PRIVATE 
    window 
    gl_context 
    vmath
    sandbox_utils 
    ve001
    glfw
    sh_config
    logger
    CLI11::CLI11
)

target_precompile_headers(ve001-benchmark PRIVATE "<CLI/CLI.hpp>")

add_dependencies(ve001-benchmark compile_and_copy_shaders_dirs)