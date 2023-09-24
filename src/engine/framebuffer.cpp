#include "framebuffer.h"

#include <glad/glad.h>

#include <algorithm>

using namespace vmath;
using namespace ve001;

Framebuffer::Framebuffer(i32 width, i32 height, const std::vector<Attachment>& attachments) 
    : _width(width), _height(height), _texture_ids(attachments.size(), 0U) {
    for (auto& attachment : attachments) {
        _attachments.emplace_back(attachment.id, Texture(_width, _height, attachment.texture_params));
    }
}

void Framebuffer::init() {
    glCreateFramebuffers(1, &_fbo_id);

    glCreateTextures(GL_TEXTURE_2D, _texture_ids.size(), _texture_ids.data());

    std::size_t i{ 0U };
    for (auto& attachment : _attachments) {
        attachment.texture.init(_texture_ids[i]);
        glNamedFramebufferTexture(_fbo_id, attachment.id, _texture_ids[i], 0);
        ++i; 
    }
}

bool Framebuffer::resize(i32 width, i32 height) {
    if (_width == width && _height == height) {
        return false;
    }

    _width = width;
    _height = height;

    glDeleteTextures(_texture_ids.size(), _texture_ids.data());
    glCreateTextures(GL_TEXTURE_2D, _texture_ids.size(), _texture_ids.data());

    std::size_t i{ 0U };
    for (auto& attachment : _attachments) {
        attachment.texture.resize(_texture_ids[i], _width, _height);
        glNamedFramebufferTexture(_fbo_id, attachment.id, _texture_ids[i], 0);
        ++i;
    }

    return true;
}

void Framebuffer::setDrawBuffer(u32 id) {
    glNamedFramebufferDrawBuffer(_fbo_id, id);
}
void Framebuffer::setReadBuffer(u32 id) {
    glNamedFramebufferReadBuffer(_fbo_id, id);
}

void Framebuffer::deinit() {
    glDeleteTextures(_texture_ids.size(), _texture_ids.data());
    glDeleteFramebuffers(1, &_fbo_id);

    _attachments.clear();
    _texture_ids.clear();
}

void Framebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo_id);
}
void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}