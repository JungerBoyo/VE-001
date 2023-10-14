#ifndef VE001_VERTEX_H
#define VE001_VERTEX_H

#include <vmath/vmath.h>

namespace ve001 {

struct Vertex {
    vmath::Vec3f32 position;
    vmath::Vec3f32 texcoord;
};

};

#endif