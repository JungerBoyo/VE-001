#include "texture.h"

#include <glad/glad.h>

#include <numeric>
#include <cmath>

using namespace vmath;
using namespace ve001;

Texture::Texture(i32 width, i32 height, std::optional<TextureParams> params)
    : _width(width), _height(height), _params(params.value_or(TextureParams{
        .internal_format = GL_SRGB8_ALPHA8, // GL_RGBA8
        .format = GL_RGBA, 
        .type = GL_UNSIGNED_BYTE,
        .wrap_s = GL_REPEAT,
        .wrap_t = GL_REPEAT,
        .min_filter = GL_NEAREST, 
        .mag_filter = GL_NEAREST
    })) {
}
void Texture::init(u32 tex_id) {
    _tex_id = tex_id;
    const i32 levels = _params.gen_mip_map ? 1U + std::floor(std::log2(std::max(
        static_cast<f32>(_width), 
        static_cast<f32>(_height)
    ))) : 1U;

    glTextureStorage2D(
        _tex_id, levels, _params.internal_format, //GL_RGBA8
        _width, _height
    );
    glTextureParameteri(_tex_id, GL_TEXTURE_WRAP_S, _params.wrap_s);
    glTextureParameteri(_tex_id, GL_TEXTURE_WRAP_T, _params.wrap_t);
    glTextureParameteri(_tex_id, GL_TEXTURE_MIN_FILTER, _params.min_filter);
    glTextureParameteri(_tex_id, GL_TEXTURE_MAG_FILTER, _params.mag_filter);
    if (_params.gen_mip_map) {
        glGenerateTextureMipmap(_tex_id);
    }
    if (_params.set_aux_params != nullptr) {
        _params.set_aux_params(_tex_id);
    }
}

void Texture::init() {
    glCreateTextures(GL_TEXTURE_2D, 1, &_tex_id);
    init(_tex_id);
}

void Texture::resize(u32 tex_id, i32 width, i32 height) {
    _width = width;
    _height = height;
    init(tex_id);
}
void Texture::resize(i32 width, i32 height) {
    if (_width == width && _height == height) {
        return;
    }

    deinit();
    
    _width = width;
    _height = height;

    init();
}

void Texture::bind(u32 texture_unit) {
    glBindTextureUnit(texture_unit, _tex_id);
}

void Texture::deinit() {
    glDeleteTextures(1, &_tex_id);
}