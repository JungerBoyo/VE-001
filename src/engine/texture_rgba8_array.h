#ifndef VE001_TEXTURE_RGBA8_ARRAY_H
#define VE001_TEXTURE_RGBA8_ARRAY_H

#include <vmath/vmath_types.h>

#include "errors.h"

using namespace vmath;

namespace ve001 {

struct TextureRGBA8Array {
private:
    u32 tex_id{ 0U };
    i32 width{ 0 };
    i32 height{ 0 };
    i32 depth{ 0 };
public:
    static TextureRGBA8Array init(i32 width, i32 height, i32 depth);

    Error writeData(const void* src, i32 index);

    void bind(u32 texture_unit);

    void deinit();
};

}

#endif