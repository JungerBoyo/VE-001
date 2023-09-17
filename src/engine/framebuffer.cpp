#include "framebuffer.h"

#include <glad/glad.h>

#include <algorithm>

using namespace vmath;
using namespace ve001;

static void storeAndAttach(u32 fbo_id, i32 width, i32 height, Framebuffer::Attachment attachment, u32 tex_id) {
    glTextureStorage2D(
        tex_id, 
        1, 
        attachment.internal_format,
        width,
        height
    );
    glNamedFramebufferTexture(
        fbo_id,
        attachment.attachment_id,
        tex_id,
        0
    );            
}

void Framebuffer::init(
    i32 width,
    i32 height,
    const std::vector<Attachment>& color_attachments, 
    bool depth_attachment,
    bool stencil_attachment
) {
    _width = width;
    _height = height;

    std::vector<u32> texture_ids(
        color_attachments.size() + 
        static_cast<std::size_t>(depth_attachment) +
        static_cast<std::size_t>(stencil_attachment),
        0U
    );
    glCreateTextures(GL_TEXTURE_2D, texture_ids.size(), texture_ids.data());

    std::size_t i{ 0U };
    if (!color_attachments.empty()) {
        _color_attachments = color_attachments;
        for (const auto color_attachment : _color_attachments) {
            storeAndAttach(_fbo_id, width, height, color_attachment, texture_ids[i]);
            ++i;
        }
    }

    std::copy(
        texture_ids.cbegin(), 
        std::next(texture_ids.cbegin(), i), 
        _color_attachment_textures.begin()
    );

    if (depth_attachment) {
        const Attachment attachment {
            .attachment_id = GL_DEPTH_ATTACHMENT, 
            .internal_format = GL_DEPTH_COMPONENT24
        };
        storeAndAttach(_fbo_id, width, height, attachment, texture_ids[i]);
        _depth_attachment_texture = texture_ids[i++];
    }
    if (stencil_attachment) {
        const Attachment attachment {
            .attachment_id = GL_STENCIL_ATTACHMENT, 
            .internal_format = GL_STENCIL_INDEX8
        };
        storeAndAttach(_fbo_id, width, height, attachment, texture_ids[i]);
        _stencil_attachment_texture = texture_ids[i++];
    }
}

void Framebuffer::resize(i32 width, i32 height) {
    if (_width == width && _height == height) {
        return;
    }

    _width = width;
    _height = height;

    const bool depth_attachment = _depth_attachment_texture != 0U;
    const bool stencil_attachment = _stencil_attachment_texture != 0U;
    
    if (depth_attachment) {
        _color_attachment_textures.push_back(_depth_attachment_texture);
    }
    if (stencil_attachment) {
        _color_attachment_textures.push_back(_stencil_attachment_texture);
    }

    glDeleteTextures(_color_attachment_textures.size(), _color_attachment_textures.data());

    glCreateTextures(
        GL_TEXTURE_2D, 
        _color_attachment_textures.size(),
        _color_attachment_textures.data()
    );

    std::size_t i{ 0U };
    if (!_color_attachments.empty()) {
        for (const auto color_attachment : _color_attachments) {
            storeAndAttach(_fbo_id, width, height, color_attachment, _color_attachment_textures[i]);
            ++i;
        }
    }

    if (depth_attachment) {
        const Attachment attachment {
            .attachment_id = GL_DEPTH_ATTACHMENT, 
            .internal_format = GL_DEPTH_COMPONENT24
        };
        storeAndAttach(_fbo_id, width, height, attachment, _color_attachment_textures.back());
        _depth_attachment_texture = _color_attachment_textures.back();
        _color_attachment_textures.pop_back();
    }
    if (stencil_attachment) {
        const Attachment attachment {
            .attachment_id = GL_STENCIL_ATTACHMENT, 
            .internal_format = GL_STENCIL_INDEX8
        };
        storeAndAttach(_fbo_id, width, height, attachment, _color_attachment_textures.back());
        _stencil_attachment_texture = _color_attachment_textures.back();
        _color_attachment_textures.pop_back();
    }
}

void Framebuffer::deinit() {
    const bool depth_attachment = _depth_attachment_texture != 0U;
    const bool stencil_attachment = _stencil_attachment_texture != 0U;

    _color_attachment_textures.push_back(_depth_attachment_texture);
    _color_attachment_textures.push_back(_stencil_attachment_texture);

    glDeleteTextures(_color_attachment_textures.size(), _color_attachment_textures.data());
    
    glDeleteFramebuffers(1, &_fbo_id);

    _width = 0U;
    _height = 0U;
    _color_attachment_textures.clear();
    _depth_attachment_texture = 0U;
    _stencil_attachment_texture = 0U;
    _color_attachments.clear();
}