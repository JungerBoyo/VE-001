#ifndef VE001_FRAMEBUFFER_H
#define VE001_FRAMEBUFFER_H

#include <vmath/vmath_types.h>

#include <vector>
#include <optional>

using namespace vmath;

namespace ve001 {

struct Framebuffer {
    u32 _fbo_id{ 0U };

    struct Attachment {
        u32 attachment_id{ 0U };
        u32 internal_format{ 0U };
    };

    std::vector<Attachment> _color_attachments;

    std::vector<u32> _color_attachment_textures;
    u32 _depth_attachment_texture{ 0U };
    u32 _stencil_attachment_texture{ 0U };

    i32 _width{ 0 };
    i32 _height{ 0 };

    void init(
        i32 width,
        i32 height,
        const std::vector<Attachment>& color_attachments, 
        bool depth_attachment,
        bool stencil_attachment
    );

    void resize(i32 width, i32 height);

    void deinit();
};

}

#endif