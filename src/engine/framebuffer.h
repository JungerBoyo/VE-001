#ifndef VE001_FRAMEBUFFER_H
#define VE001_FRAMEBUFFER_H

#include <vmath/vmath_types.h>

#include <vector>
#include <optional>

#include "texture.h"

namespace ve001 {

struct Framebuffer {
    struct Attachment {
        vmath::u32 id;
        TextureParams texture_params;
    };
    
    struct InternalAttachment {
        vmath::u32 id;
        Texture texture;
    };

    vmath::u32 _fbo_id{ 0U };

    vmath::i32 _width{ 0U };
    vmath::i32 _height{ 0U };

    std::vector<InternalAttachment> _attachments;
    std::vector<Texture> _textures;
    std::vector<vmath::u32> _texture_ids; // stored here to avoid memory allocation upon resize

    Framebuffer() = default;
    Framebuffer(vmath::i32 width, vmath::i32 height, const std::vector<Attachment>& attachments);

    void init();
    void initRaw();

    bool resize(vmath::i32 width, vmath::i32 height);

    void bindTexToAttachment(vmath::u32 attachment_id, vmath::u32 tex_id);
    void bindTexLayerToAttachment(vmath::u32 attachment_id, vmath::u32 tex_id, vmath::i32 layer);

    void setDrawBuffer(vmath::u32 id);
    void setReadBuffer(vmath::u32 id);

    void bind();
    void unbind();
    void deinit();
};

}

#endif