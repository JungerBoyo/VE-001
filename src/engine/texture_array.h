#ifndef VE001_TEXTURE_ARRAY_H
#define VE001_TEXTURE_ARRAY_H

#include <vmath/vmath_types.h>

#include "errors.h"
#include "texture_params.h"

#include <optional>

using namespace vmath;

namespace ve001 {

struct TextureArray {
    u32 _tex_id{ 0U };
    i32 _width{ 0U };
    i32 _height{ 0U };
    i32 _levels{ 0U };
    i32 _depth{ 0U };
    u32 _size{ 0U };
    TextureParams _params;

    TextureArray() = default;
    TextureArray(i32 width, i32 height, i32 depth, std::optional<TextureParams> params = std::nullopt); 

    void init();

    u32 pushBack(const void* data);

    void bind(u32 texture_unit);

    void deinit();
};

}

#endif