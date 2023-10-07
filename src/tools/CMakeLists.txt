add_executable(gl-caps-dump gl_caps_dumper.cpp)

target_link_libraries(gl-caps-dump PRIVATE
    window
    gl_context
)

find_package(CLI11 REQUIRED)

add_executable(vox2ve0 vox2ve0.cpp)

target_link_libraries(vox2ve0 PRIVATE
    vmath
    CLI11::CLI11
    spdlog::spdlog
)

