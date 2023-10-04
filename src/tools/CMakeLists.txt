add_executable(gl-caps-dump gl_caps_dumper.cpp)

target_link_libraries(gl-caps-dump PRIVATE
    window
    gl_context
)