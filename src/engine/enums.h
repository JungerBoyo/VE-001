#ifndef VE001_ENUMS_H
#define VE001_ENUMS_H

#include <vmath/vmath_types.h>

namespace ve001 {

enum Face : vmath::u32 {
    X_POS, X_NEG, 
    Y_POS, Y_NEG, 
    Z_POS, Z_NEG
};

enum Error : vmath::u32 {
    NO_ERROR = 0x0U,
    GPU_ALLOCATION_FAILED = 0x01U,
    CPU_ALLOCATION_FAILED = 0x02U,
    GPU_BUFFER_MAPPING_FAILED = 0x04U,
    FENCE_WAIT_FAILED = 0x08U,
    SHADER_ATTACH_FAILED = 0x10U,
    CHUNK_DATA_STREAMER_THREAD_ALLOCATION_FAILED = 0x20U,
    CHUNK_DATA_STREAMER_THREAD_INITIALIZATION_FAILED = 0x20U,
};

inline Error operator|(Error lhs, Error rhs) {
    return static_cast<Error>(static_cast<vmath::u32>(lhs) | static_cast<vmath::u32>(rhs));
}
inline Error operator&(Error lhs, Error rhs) {
    return static_cast<Error>(static_cast<vmath::u32>(lhs) & static_cast<vmath::u32>(rhs));
}
inline Error& operator|=(Error& lhs, Error rhs) {
    return lhs = lhs | rhs;
}

}


#endif