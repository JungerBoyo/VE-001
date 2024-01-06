#ifndef VE001_GPU_BUFFER_H
#define VE001_GPU_BUFFER_H

#include <vmath/vmath_types.h>

#include "enums.h"

namespace ve001 {

struct GPUBuffer {
    vmath::u32 _id{ 0U };
    vmath::u32 _size;

    GPUBuffer(vmath::u32 size) : _size(size) {}

    Error init() noexcept;
    void write(const void* data) noexcept;
    void write(const void* data, vmath::u32 offset, vmath::u32 size) noexcept;
    void read(void* data, vmath::u32 offset, vmath::u32 size) noexcept;
    void bind(vmath::u32 target, vmath::u32 binding) noexcept;
    void deinit() noexcept;
};

}

#endif