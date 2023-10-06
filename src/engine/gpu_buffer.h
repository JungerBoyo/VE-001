#ifndef VE001_GPU_BUFFER_H
#define VE001_GPU_BUFFER_H

#include <vmath/vmath_types.h>

namespace ve001 {

struct GPUBuffer {
    vmath::u32 _id{ 0U };
    vmath::u32 _size;

    GPUBuffer(vmath::u32 size) : _size(size) {}

    void init();
    void init(vmath::u32 id);
    void write(const void* data);
    void write(const void* data, vmath::u32 offset, vmath::u32 size);
    void bind(vmath::u32 target, vmath::u32 binding);
    void deinit();
};

}

#endif