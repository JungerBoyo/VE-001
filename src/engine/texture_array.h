#ifndef VE001_TEXTURE_ARRAY_H
#define VE001_TEXTURE_ARRAY_H

#include <vmath/vmath_types.h>

#include "errors.h"

#include <optional>

using namespace vmath;

namespace ve001 {

struct TextureArray {
    struct Params {
        u32 internal_format;
        u32 format;
        u32 type;
        u32 wrap_s;
        u32 wrap_t;
        u32 min_filter;
        u32 mag_filter;
    };

    u32 _tex_id{ 0U };
    i32 _width{ 0U };
    i32 _height{ 0U };
    i32 _depth{ 0U };
    u32 _size{ 0U };
    Params _params;

    TextureArray() = default;
    TextureArray(i32 width, i32 height, i32 depth, std::optional<Params> params = std::nullopt); 

    void init();

    // Error writeData(const void* src, i32 index);

    u32 pushBack(const void* data);

    void bind(u32 texture_unit);

    void deinit();
};

}

#endif