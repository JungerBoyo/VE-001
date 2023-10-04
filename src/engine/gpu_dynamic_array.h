#ifndef VE001_GPU_DYNAMIC_ARRAY_H
#define VE001_GPU_DYNAMIC_ARRAY_H

#include "vmath/vmath_types.h"
#include "errors.h"

namespace ve001 {

struct GPUDynamicArray {
    static constexpr vmath::f32 GROW_FACTOR{ 2.F };

    vmath::u32 _ssbo_id{ 0U };
    void* _ssbo_ptr{ nullptr };

    vmath::u32 _size{ 0U };
    vmath::u32 _capacity{ 0U };
    vmath::u32 _element_size{ 0U };

    GPUDynamicArray(vmath::u32 element_size) 
        : _element_size(element_size) {}    

    Error init(vmath::u32 capacity) noexcept;
    void deinit() noexcept;

    Error pushBack(const void* data) noexcept;
    void popBack() noexcept;

    Error reserve(vmath::u32 capacity) noexcept;
    Error resize(vmath::u32 size) noexcept;

    Error write(const void* data, vmath::u32 count, vmath::u32 offset) noexcept;

    void bind(vmath::u32 binding_index) noexcept;
};

}

#endif