#ifndef VE001_CUBETEXTURE_ARRAY_H
#define VE001_CUBETEXTURE_ARRAY_H

#include <vmath/vmath_types.h>

#include "errors.h"
#include "texture_params.h"

#include <optional>

namespace ve001 {

struct CubeTextureArray {
    vmath::u32 _tex_id{ 0U };
    vmath::i32 _width{ 0U };
    vmath::i32 _height{ 0U };
    vmath::i32 _levels{ 0U };
    vmath::i32 _depth{ 0U };
    vmath::u32 _size{ 0U };
    TextureParams _params;

    CubeTextureArray(vmath::i32 width, vmath::i32 height, vmath::i32 depth, std::optional<TextureParams> params = std::nullopt); 

    void init();

    void pushBack(
        const void* pos_x, const void* neg_x, 
        const void* pos_y, const void* neg_y, 
        const void* pos_z, const void* neg_z
    );
    void emplace();

    const auto size() const { return _size; }

    void bind(vmath::u32 texture_unit);

    void deinit();
};

}

#endif