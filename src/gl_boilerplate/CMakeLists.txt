add_library(gl_boilerplate STATIC
    shader.h
    shader.cpp
)

target_link_libraries(gl_boilerplate PRIVATE glad::glad)