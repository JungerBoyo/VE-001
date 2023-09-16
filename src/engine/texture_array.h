#ifndef VE001_TEXTURE_ARRAY_H
#define VE001_TEXTURE_ARRAY_H

#include <vmath/vmath_types.h>

#include "errors.h"

using namespace vmath;

namespace ve001 {

struct TextureArray {
    u32 _tex_id{ 0U };
    i32 _width{ 0U };
    i32 _height{ 0U };
    i32 _depth{ 0U };
    u32 _size{ 0U };

    TextureArray() = default;
    TextureArray(i32 width, i32 height, i32 depth) 
        : _width(width), _height(height), _depth(depth) {}

    void init();

    // Error writeData(const void* src, i32 index);

    u32 pushBack(const void* data);

    void bind(u32 texture_unit);

    void deinit();
};

}

#endif