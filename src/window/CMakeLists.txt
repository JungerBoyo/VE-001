add_library(window STATIC 
    window.h
    window_glfw3.cpp
    window_glfw3.tpp
)
target_link_libraries(window PRIVATE glfw vmath logger)
