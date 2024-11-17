#!/bin/bash

is_installed() {
    pacman -Q "$1" &> /dev/null
}

packages=(cmake ninja)
for p in "${packages[@]}"; do
	if ! is_installed "${p}"; then
		sudo pacman -S "${p}"
	fi
done

python -m venv .venv
source .venv/bin/activate
pip install conan

conan install . --output-folder=build-rel-glfw3-testing -g CMakeToolchain -pr:b conanprofile.linux.txt -pr:h conanprofile.linux.txt --build=missing

cmake -S . -B build-rel-glfw3-testing -G Ninja --toolchain conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DENGINE_TEST=ON

cmake --build build-rel-glfw3-testing
