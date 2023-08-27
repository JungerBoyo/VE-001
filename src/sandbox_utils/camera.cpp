#include "camera.h"

using namespace vmath;
using namespace ve001;

Mat4f32 Camera::lookAt() {
    rhs_dir = Vec3f32::normalize(cross(neg_looking_dir, world_up_dir));
    up_dir = Vec3f32::normalize(cross(rhs_dir, neg_looking_dir));

    // construct lookAt matrix (camera basis matrix * camera translation matrix)
    return Mat4f32(
        Vec4f32(rhs_dir[0], up_dir[0], -neg_looking_dir[0], 0.F),
        Vec4f32(rhs_dir[1], up_dir[1], -neg_looking_dir[1], 0.F),
        Vec4f32(rhs_dir[2], up_dir[2], -neg_looking_dir[2], 0.F),
        Vec4f32(
            -Vec3f32::dot(rhs_dir, position),
            -Vec3f32::dot(up_dir, position),
             Vec3f32::dot(neg_looking_dir, position),
             1.F
        )
    );
}
void Camera::rotate(Vec3f32 angles) {
    Mat3f32 basis(rhs_dir, up_dir, neg_looking_dir);
    neg_looking_dir = vmath::misc<f32>::rotatePoint3D(angles, neg_looking_dir, Vec3f32(0.F), basis);
}
void Camera::rotateXYPlane(Vec2f32 vec) {
    neg_looking_dir_angles[0] = vmath::misc<f32>::wrap(
        neg_looking_dir_angles[0] + vec[1], 2.F*std::numbers::pi_v<f32>, 0.F
    );
    neg_looking_dir_angles[1] = vmath::misc<f32>::wrap(
        neg_looking_dir_angles[1] + vec[0], 2.F*std::numbers::pi_v<f32>, 0.F
    );
                        // rot around Y in XZ plane (x value)  // scale by rot around X becuase vec len changed from x/z perspective
    neg_looking_dir[0] = std::cos(neg_looking_dir_angles[1]) * std::cos(neg_looking_dir_angles[0]);
    neg_looking_dir[1] = std::sin(neg_looking_dir_angles[0]);
    neg_looking_dir[2] = std::sin(neg_looking_dir_angles[1]) * std::cos(neg_looking_dir_angles[0]);

    neg_looking_dir = Vec3f32::normalize(neg_looking_dir);
}
void Camera::move(Vec3f32 vec) {
    Mat3f32 basis(rhs_dir, up_dir, neg_looking_dir);
    vec = Mat3f32::mulVec(basis, vec);
    position = Vec3f32::add(position, vec);
}