find_package(stb REQUIRED)

add_library(stb_impl STATIC stb_image.c)
target_link_libraries(stb_impl PUBLIC stb::stb)