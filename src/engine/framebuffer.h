#ifndef VE001_FRAMEBUFFER_H
#define VE001_FRAMEBUFFER_H

#include <vmath/vmath_types.h>

#include <vector>
#include <optional>

#include "texture.h"

using namespace vmath;

namespace ve001 {

struct Framebuffer {
    struct Attachment {
        u32 id;
        TextureParams texture_params;
    };
    
    struct InternalAttachment {
        u32 id;
        Texture texture;
    };

    u32 _fbo_id{ 0U };

    i32 _width{ 0U };
    i32 _height{ 0U };

    std::vector<InternalAttachment> _attachments;
    std::vector<Texture> _textures;
    std::vector<u32> _texture_ids; // stored here to avoid memory allocation upon resize

    Framebuffer(i32 width, i32 height, const std::vector<Attachment>& attachments);

    void init();
    bool resize(i32 width, i32 height);

    void setDrawBuffer(u32 id);
    void setReadBuffer(u32 id);

    void bind();
    void unbind();
    void deinit();
};

}

#endif