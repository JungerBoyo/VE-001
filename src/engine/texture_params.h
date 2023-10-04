#ifndef VE001_TEXTURE_PARAMS_H
#define VE001_TEXTURE_PARAMS_H

#include <vmath/vmath_types.h>

namespace ve001 {

struct TextureParams {
    vmath::u32 internal_format;
    vmath::u32 format;
    vmath::u32 type;
    vmath::u32 wrap_s;
    vmath::u32 wrap_t;
    vmath::u32 min_filter;
    vmath::u32 mag_filter;
    bool gen_mip_map{ false };
    void(*set_aux_params)(vmath::u32 tex_id){ nullptr };
};

}

#endif