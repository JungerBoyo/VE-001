add_library(sandbox_utils STATIC
    face_gen.h
)

target_link_libraries(sandbox_utils PRIVATE vmath)