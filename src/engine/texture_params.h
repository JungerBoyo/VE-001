#ifndef VE001_TEXTURE_PARAMS_H
#define VE001_TEXTURE_PARAMS_H

#include <vmath/vmath_types.h>

using namespace vmath;

namespace ve001 {

struct TextureParams {
    u32 internal_format;
    u32 format;
    u32 type;
    u32 wrap_s;
    u32 wrap_t;
    u32 min_filter;
    u32 mag_filter;
    bool gen_mip_map{ false };
    void(*set_aux_params)(u32 tex_id){ nullptr };
};

}

#endif