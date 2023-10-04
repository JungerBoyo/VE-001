#include "cubetexture_array.h"

#include <glad/glad.h>

#include <numeric>
#include <array>

using namespace vmath;
using namespace ve001;

CubeTextureArray::CubeTextureArray(i32 width, i32 height, i32 depth, std::optional<TextureParams> params)
    : _width(width), _height(height), _depth(6 * depth), _params(params.value_or(TextureParams{
        .internal_format = GL_SRGB8_ALPHA8, // GL_RGBA8
        .format = GL_RGBA, 
        .type = GL_UNSIGNED_BYTE,
        .wrap_s = GL_REPEAT,
        .wrap_t = GL_REPEAT,
        .min_filter = GL_NEAREST, 
        .mag_filter = GL_NEAREST
    })) {
}

void CubeTextureArray::init() {
    glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &_tex_id);
    
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

void CubeTextureArray::pushBack(
    const void* pos_x, const void* neg_x, 
    const void* pos_y, const void* neg_y, 
    const void* pos_z, const void* neg_z
) {
    if (_size == _depth) {
        return;
    }
   
    std::array<const void*, 6> data{{
        pos_x, neg_x, 
        pos_y, neg_y, 
        pos_z, neg_z
    }};
    for (const void* face_ptr : data) {
        glTextureSubImage3D(
            _tex_id, 0, 
            0, 0, _size++, 
            _width, _height, 1,
            _params.format, _params.type,
            face_ptr 
        );
    }
}

void CubeTextureArray::emplace() {
    if (_size == _depth) {
        return;
    }
    _size += 6;
}


void CubeTextureArray::bind(u32 texture_unit) {
    glBindTextureUnit(texture_unit, _tex_id);
}

void CubeTextureArray::deinit() {
    glDeleteTextures(1, &_tex_id);
}