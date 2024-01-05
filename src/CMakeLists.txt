find_package(glad REQUIRED)
find_package(spdlog REQUIRED)

if (NOT "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    find_package(FastNoise2 REQUIRED)
endif()

find_package(CLI11 REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(window)
add_subdirectory(engine)
add_subdirectory(sandbox_utils)
add_subdirectory(logger)


if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    target_compile_definitions(logger PRIVATE WINDOWS)
    target_compile_definitions(sandbox_utils PRIVATE WINDOWS)
endif()

add_library(gl_context STATIC gl_context.h gl_context.cpp)
target_link_libraries(gl_context PUBLIC glad::glad)
target_link_libraries(gl_context PRIVATE glfw)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_definitions(gl_context PRIVATE DEBUG)
endif()

target_link_libraries(gl_context PRIVATE logger)

add_executable(ve001-benchmark ve001_benchmark.cpp testing_context.h)

if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    target_compile_definitions(ve001-benchmark PRIVATE WINDOWS)
endif()

target_link_libraries(ve001-benchmark PRIVATE 
    window 
    gl_context 
    vmath
    sandbox_utils 
    ve001
    glfw
    logger
    CLI11::CLI11
)

add_dependencies(ve001-benchmark compile_and_copy_shaders_dirs)