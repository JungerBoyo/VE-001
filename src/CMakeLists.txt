find_package(fmt REQUIRED)
find_package(glad REQUIRED)
find_package(stb REQUIRED)

if (USE_SDL2)
    add_compile_definitions(${PROJECT_NAME}_USE_SDL2=1)
    find_package(SDL2 REQUIRED)
elseif (USE_GLFW3)
    add_compile_definitions(${PROJECT_NAME}_USE_GLFW3=1)
    find_package(glfw3 REQUIRED)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(types INTERFACE types.h)
link_libraries(types)

add_subdirectory(window)

add_library(gl_context STATIC gl_context.h gl_context.cpp)
target_link_libraries(gl_context PUBLIC glad::glad)
if (USE_SDL2)
    target_link_libraries(gl_context PRIVATE SDL2::SDL2)
elseif (USE_GLFW3)
    target_link_libraries(gl_context PRIVATE glfw)
endif()

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt window gl_context vmath)




