#include "camera.h"

using namespace vmath;
using namespace ve001;

Mat4f32 Camera::lookAt() {
    rhs_dir = Vec3f32::normalize(cross(looking_dir, world_up_dir));
    up_dir = Vec3f32::normalize(cross(rhs_dir, looking_dir));

    // construct lookAt matrix (camera basis matrix * camera translation matrix)
    return Mat4f32(
        Vec4f32(rhs_dir[0], up_dir[0], -looking_dir[0], 0.F),
        Vec4f32(rhs_dir[1], up_dir[1], -looking_dir[1], 0.F),
        Vec4f32(rhs_dir[2], up_dir[2], -looking_dir[2], 0.F),
        Vec4f32(
            -Vec3f32::dot(rhs_dir, position),
            -Vec3f32::dot(up_dir, position),
             Vec3f32::dot(looking_dir, position),
             1.F
        )
    );
}
void Camera::rotate(Vec3f32 angles) {
    Mat3f32 basis(rhs_dir, up_dir, looking_dir);
    looking_dir = vmath::misc<f32>::rotatePoint3D(angles, looking_dir, Vec3f32(0.F), basis);
    looking_dir_angles = Vec3f32::add(looking_dir_angles, angles);
}
void Camera::rotateXYPlane(Vec2f32 vec) {
    looking_dir_angles[0] = vmath::misc<f32>::wrap(
        looking_dir_angles[0] + vec[1], 2.F*std::numbers::pi_v<f32>, 0.F
    );
    looking_dir_angles[1] = vmath::misc<f32>::wrap(
        looking_dir_angles[1] + vec[0], 2.F*std::numbers::pi_v<f32>, 0.F
    );
                        // rot around Y in XZ plane (x value)  // scale by rot around X becuase vec len changed from x/z perspective
    looking_dir[0] = std::cos(looking_dir_angles[1]) * std::cos(looking_dir_angles[0]);
    looking_dir[1] = std::sin(looking_dir_angles[0]);
    looking_dir[2] = std::sin(looking_dir_angles[1]) * std::cos(looking_dir_angles[0]);

    looking_dir = Vec3f32::normalize(looking_dir);
}
void Camera::move(Vec3f32 vec) {
    Mat3f32 basis(rhs_dir, up_dir, looking_dir);
    vec = Mat3f32::mulVec(basis, vec);
    position = Vec3f32::add(position, vec);
}