add_executable(gl-caps-dump gl_caps_dumper.cpp)

target_link_libraries(gl-caps-dump PRIVATE
    window
    gl_context
)

find_package(CLI11 REQUIRED)
find_package(zstd REQUIRED)

add_executable(vox2ve0 vox2ve0.cpp)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release") 
    target_link_libraries(vox2ve0 PRIVATE
        vmath
        CLI11::CLI11
        spdlog::spdlog
        zstd::libzstd_shared
    )
else()
    target_link_libraries(vox2ve0 PRIVATE
        vmath
        CLI11::CLI11
        spdlog::spdlog
        zstd::libzstd_static
    )
    target_compile_definitions(vox2ve0 PRIVATE DEBUG)
endif()

