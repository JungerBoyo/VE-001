#ifndef VE001_FACE_GEN_H
#define VE001_FACE_GEN_H

#include <vmath/vmath.h>
#include <array>

namespace ve001 {

struct Vertex {
    Vec3f32 position;
    Vec3f32 texcoord;
};

constexpr Vec3f32 genPosition(u8 bin, Vec3f32 scale) { switch (bin) {
    case 0: return Vec3f32(0.F,         0.F,        0.F);       // --- 
    case 1: return Vec3f32(0.F,         0.F,        scale[2]);  // --+
    case 2: return Vec3f32(0.F,         scale[1],   0.F);       // -+-
    case 3: return Vec3f32(0.F,         scale[1],   scale[2]);  // -++
    case 4: return Vec3f32(scale[0],    0.F,        0.F);       // +--
    case 5: return Vec3f32(scale[0],    0.F,        scale[2]);  // +-+
    case 6: return Vec3f32(scale[0],    scale[1],   0.F);       // ++-
    case 7: return Vec3f32(scale[0],    scale[1],   scale[2]);  // +++

    default: return Vec3f32(0.F, 0.F, 0.F);
}}

constexpr std::array<Vertex, 6> genFace(u8 face, Vec3f32 offset, Vec3f32 scale, Vec2f32 face_scale) {
    std::array<Vertex, 6> faces;
    switch (face) {
    case 0: //POS_X: 
        faces[0].position = Vec3f32::add(offset, genPosition(5U, scale)); 
        faces[1].position = Vec3f32::add(offset, genPosition(4U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(6U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(5U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(6U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(7U, scale));

    break;
    case 1: //NEG_X: 
        faces[0].position = Vec3f32::add(offset, genPosition(0U, scale));
        faces[1].position = Vec3f32::add(offset, genPosition(1U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(3U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(0U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(3U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(2U, scale));
    break;
    case 2: //POS_Y: 
        faces[0].position = Vec3f32::add(offset, genPosition(2U, scale));
        faces[1].position = Vec3f32::add(offset, genPosition(3U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(7U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(2U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(7U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(6U, scale));
    break;
    case 3: //NEG_Y: 
        faces[0].position = Vec3f32::add(offset, genPosition(4U, scale));
        faces[1].position = Vec3f32::add(offset, genPosition(5U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(1U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(4U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(1U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(0U, scale));
    break;
    case 4: //POS_Z: 
        faces[0].position = Vec3f32::add(offset, genPosition(1U, scale));
        faces[1].position = Vec3f32::add(offset, genPosition(5U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(7U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(1U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(7U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(3U, scale));
    break;
    case 5: //NEG_Z: 
        faces[0].position = Vec3f32::add(offset, genPosition(4U, scale));
        faces[1].position = Vec3f32::add(offset, genPosition(0U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(2U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(4U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(2U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(6U, scale));
    break;
    }

    faces[0].texcoord = Vec3f32(0.F,            0.F,            static_cast<f32>(face));
    faces[1].texcoord = Vec3f32(face_scale[0],  0.F,            static_cast<f32>(face));
    faces[2].texcoord = Vec3f32(face_scale[0],  face_scale[1],  static_cast<f32>(face));
    faces[3].texcoord = Vec3f32(0.F,            0.F,            static_cast<f32>(face));
    faces[4].texcoord = Vec3f32(face_scale[0],  face_scale[1],  static_cast<f32>(face));
    faces[5].texcoord = Vec3f32(0.F,            face_scale[1],  static_cast<f32>(face));

    return faces;
}

}

#endif