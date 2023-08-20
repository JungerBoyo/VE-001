#include "texture_rgba8_array.h"

#include <glad/glad.h>

using namespace vmath;
using namespace ve001;

TextureRGBA8Array TextureRGBA8Array::init(i32 width, i32 height, i32 depth) {
    TextureRGBA8Array self{};

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &self.tex_id);

    glTextureStorage3D(
        self.tex_id, 1, GL_RGBA8,
        width, height, depth
    );
    glTextureParameteri(self.tex_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(self.tex_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(self.tex_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(self.tex_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    self.width = width;
    self.height = height;
    self.depth = depth;

    return self;
}

Error TextureRGBA8Array::writeData(const void* src, i32 index) {
    if (index >= depth) {
        return Error::TEXTURE_INDEX_OUT_OF_RANGE;
    }

    glTextureSubImage3D(
        tex_id, 0, 
        0, 0, index, 
        width, height, 1,
        GL_RGBA, GL_UNSIGNED_BYTE,
        src
    );

    return Error::NO_ERROR;
}

void TextureRGBA8Array::bind(u32 texture_unit) {
    glBindTextureUnit(texture_unit, tex_id);
}

void TextureRGBA8Array::deinit() {
    glDeleteTextures(1, &tex_id);
}