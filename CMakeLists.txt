cmake_minimum_required(VERSION 3.20)

### before-project config ###

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

#############################


### project setup ###

project(VE001
    VERSION 0.0.1
    HOMEPAGE_URL "https://github.com/JungerBoyo/VE-001"
)

######################


### sub dirs ###

add_subdirectory(shaders)
add_subdirectory(src)

################

### add conan install as dependency ###


### package project ###
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "./")
install(DIRECTORY ${PROJECT_SOURCE_DIR}/shaders/bin DESTINATION "shaders/")
install(DIRECTORY ${PROJECT_SOURCE_DIR}/shaders/src DESTINATION "shaders/")
install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/textures DESTINATION "assets/")

include(CPack)
########################