if (USE_GLFW3)
    add_library(window STATIC 
        window.h
        window_glfw3.cpp
        window_glfw3.tpp
    )
    target_link_libraries(window PRIVATE glfw logger)
elseif (USE_SDL2)
    add_library(window STATIC 
        window.h
        window_SDL2.cpp
        window_SDL2.tpp
    )
    target_link_libraries(window PRIVATE SDL2::SDL2 logger)
endif()

