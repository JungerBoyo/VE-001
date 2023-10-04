#ifndef VE001_TEXTURE_H
#define VE001_TEXTURE_H

#include <vmath/vmath_types.h>

#include "errors.h"
#include "texture_params.h"

#include <optional>

namespace ve001 {

struct Texture {
    vmath::u32 _tex_id{ 0U };
    vmath::i32 _width{ 0U };
    vmath::i32 _height{ 0U };
    TextureParams _params;

    Texture() = default;
    Texture(vmath::i32 width, vmath::i32 height, std::optional<TextureParams> params = std::nullopt); 

    void init();
    void init(vmath::u32 tex_id);
    void bind(vmath::u32 texture_unit);
    void resize(vmath::u32 tex_id, vmath::i32 width, vmath::i32 height);
    void resize(vmath::i32 width, vmath::i32 height);
    void deinit();
};

}

#endif