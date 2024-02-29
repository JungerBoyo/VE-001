conan install . --output-folder=build-rel -g CMakeToolchain -pr:b conanprofile.linux.txt -pr:h conanprofile.linux.txt -o --build=missing && \
cmake -S . -B build-rel-glfw3 -G Ninja -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
cmake --build build-rel-glfw3


