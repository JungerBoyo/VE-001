conan install . --output-folder=build-rel && \
cmake -S . -B build-rel -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
cmake --build build-rel