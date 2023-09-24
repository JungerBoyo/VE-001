#ifndef VE001_TEXTURE_H
#define VE001_TEXTURE_H

#include <vmath/vmath_types.h>

#include "errors.h"
#include "texture_params.h"

#include <optional>

using namespace vmath;

namespace ve001 {

struct Texture {
    u32 _tex_id{ 0U };
    i32 _width{ 0U };
    i32 _height{ 0U };
    TextureParams _params;

    Texture() = default;
    Texture(i32 width, i32 height, std::optional<TextureParams> params = std::nullopt); 

    void init();
    void init(u32 tex_id);
    void bind(u32 texture_unit);
    void resize(u32 tex_id, i32 width, i32 height);
    void resize(i32 width, i32 height);
    void deinit();
};

}

#endif