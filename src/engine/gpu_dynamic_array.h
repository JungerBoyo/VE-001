#ifndef VE001_GPU_DYNAMIC_ARRAY_H
#define VE001_GPU_DYNAMIC_ARRAY_H

#include "vmath/vmath_types.h"
#include "errors.h"

using namespace vmath;

namespace ve001 {

struct GPUDynamicArray {
    static constexpr f32 GROW_FACTOR{ 2.F };

    u32 _ssbo_id{ 0U };
    void* _ssbo_ptr{ nullptr };

    u32 _size{ 0U };
    u32 _capacity{ 0U };
    u32 _element_size{ 0U };

    GPUDynamicArray(u32 element_size) 
        : _element_size(element_size) {}    

    Error init(u32 capacity) noexcept;
    void deinit() noexcept;

    Error pushBack(u32& id, const void* data) noexcept;
    void popBack() noexcept;

    Error reserve(u32 capacity) noexcept;
    Error resize(u32 size) noexcept;

    Error write(void* data, u32 count, u32 offset) noexcept;

    void bind(u32 binding_index) noexcept;
};

}

#endif