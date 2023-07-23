add_library(logger STATIC logger.h logger.cpp)
target_link_libraries(logger PUBLIC spdlog::spdlog)