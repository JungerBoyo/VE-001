#include "texture_array.h"

#include <glad/glad.h>

#include <numeric>

using namespace vmath;
using namespace ve001;

TextureArray::TextureArray(i32 width, i32 height, i32 depth, std::optional<TextureParams> params)
    : _width(width), _height(height), _depth(depth), _params(params.value_or(TextureParams{
        .internal_format = GL_SRGB8_ALPHA8, // GL_RGBA8
        .format = GL_RGBA, 
        .type = GL_UNSIGNED_BYTE,
        .wrap_s = GL_REPEAT,
        .wrap_t = GL_REPEAT,
        .min_filter = GL_NEAREST, 
        .mag_filter = GL_NEAREST
    })) {
}

void TextureArray::init() {
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &_tex_id);
    
    glTextureStorage3D(
        _tex_id, 1, _params.internal_format, //GL_RGBA8
        _width, _height, _depth
    );
    glTextureParameteri(_tex_id, GL_TEXTURE_WRAP_S, _params.wrap_s);
    glTextureParameteri(_tex_id, GL_TEXTURE_WRAP_T, _params.wrap_t);
    glTextureParameteri(_tex_id, GL_TEXTURE_MIN_FILTER, _params.min_filter);
    glTextureParameteri(_tex_id, GL_TEXTURE_MAG_FILTER, _params.mag_filter);
    if (_params.set_aux_params != nullptr) {
        _params.set_aux_params(_tex_id);
    }

}

void TextureArray::pushBack(const void* data) {
    if (_size == _depth) {
        return;
    }

    glTextureSubImage3D(
        _tex_id, 0, 
        0, 0, _size, 
        _width, _height, 1,
        _params.format, _params.type,
        data
    );

    ++_size;
}

void TextureArray::emplace() {
    ++_size;
}

void TextureArray::bind(u32 texture_unit) {
    glBindTextureUnit(texture_unit, _tex_id);
}

void TextureArray::deinit() {
    glDeleteTextures(1, &_tex_id);
}