#!/bin/bash

mkdir -p tests_out/cpu
mkdir tests_out/gpu

./engine_test.sh build-rel-glfw3-testing/src/ve001-benchmark tests_out/cpu
./engine_test.sh build-rel-glfw3-testing/src/ve001-benchmark tests_out/gpu -g

tar czf tests_out.tar.gz tests_out
