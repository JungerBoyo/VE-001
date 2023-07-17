add_library(sandbox_utils STATIC
    camera.h
    camera.cpp
)

target_link_libraries(sandbox_utils PRIVATE vmath)