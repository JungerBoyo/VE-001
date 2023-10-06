build_with_sdl2() {
    conan install . --output-folder=build-rel -pr:b conanprofile.linux.txt -pr:h conanprofile.linux.txt -o use_sdl2=True --build=missing && \
    cmake -S . -B build-rel-sdl2 -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_SDL2=ON && \
    cmake --build build-rel-sdl2
}

build_with_glfw3() {
    conan install . --output-folder=build-rel -pr:b conanprofile.linux.txt -pr:h conanprofile.linux.txt -o use_glfw3=True --build=missing && \
    cmake -S . -B build-rel-glfw3 -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_GLFW3=ON && \
    cmake --build build-rel-glfw3
}


if [ "$1" = "use_sdl2" ]; then
    build_with_sdl2
elif [ "$1" = "use_glfw3" ]; then
    build_with_glfw3
fi
