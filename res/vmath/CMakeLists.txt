add_library(vmath STATIC vmath_types.h vmath.h vmath.cpp)

target_compile_options(vmath PRIVATE
    -funroll-loops
    -fno-exceptions
    -fno-rtti
    -O2
    # -ffast-math
)