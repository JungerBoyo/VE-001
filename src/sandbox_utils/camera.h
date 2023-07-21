#ifndef VE001_CAMERA_H
#define VE001_CAMERA_H

#include <vmath/vmath.h>

using namespace vmath;

namespace ve001 {

struct Camera {
    Vec3f32 position{ 0.F };
    Vec3f32 neg_looking_dir_angles{
        std::numbers::pi_v<f32>,// vector (0,0,1) is NEGATIVE 
                                // so starting point is vector
                                // (0,0,-1). vector (0,0,1) is 
                                // rotated around X axis by 180
        -std::numbers::pi_v<f32>/2.F,// because vector (-1,0,0) is 
                                     // x axis vector so -90 in x gives
                                     // (0,0,1)
        0.F
    };

    Vec3f32 world_up_dir{ 0.F, 1.F, 0.F };
    Vec3f32 neg_looking_dir{ 0.F, 0.F, 1.F };
    Vec3f32 rhs_dir{ -1.F, 0.F, 0.F };
    Vec3f32 up_dir{ 0.F, 1.F, 0.F };

    Mat4f32 lookAt();
    void rotate(Vec3f32 angles);
    void rotateXYPlane(Vec2f32 vec);
    void move(Vec3f32 vec);
};

}

#endif