#include "texture_array.h"

#include <glad/glad.h>

#include <numeric>

using namespace vmath;
using namespace ve001;

void TextureArray::init() {
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &_tex_id);

    glTextureStorage3D(
        _tex_id, 1, GL_RGBA8,
        _width, _height, _depth
    );
    glTextureParameteri(_tex_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(_tex_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(_tex_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(_tex_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

// Error TextureArray::writeData(const void* src, i32 index) {
//     if (index >= _depth) {
//         return Error::TEXTURE_INDEX_OUT_OF_RANGE;
//     }

//     glTextureSubImage3D(
//         _tex_id, 0, 
//         0, 0, index, 
//         _width, _height, 1,
//         GL_RGBA, GL_UNSIGNED_BYTE,
//         src
//     );

//     return Error::NO_ERROR;
// }
u32 TextureArray::pushBack(const void* data) {
    if (_size == _depth) {
        return std::numeric_limits<u32>::max();
    }

    glTextureSubImage3D(
        _tex_id, 0, 
        0, 0, _size, 
        _width, _height, 1,
        GL_RGBA, GL_UNSIGNED_BYTE,
        data
    );

    return _size++;
}

void TextureArray::bind(u32 texture_unit) {
    glBindTextureUnit(texture_unit, _tex_id);
}

void TextureArray::deinit() {
    glDeleteTextures(1, &_tex_id);
}