#include "noise_func.h"

#include <iostream>

using namespace ve001;
using namespace vmath;

static u32 murmur3(u32 key, u32 seed) {
    static constexpr u32 c1{ 0xcc9e2d51U };
    static constexpr u32 c2{ 0x1b873593U };
    static constexpr u32 r1{ 15U };
    static constexpr u32 r2{ 13U };
    static constexpr u32 m{ 5U };
    static constexpr u32 n{ 0xe6546b64U };
    const u32 k{ key * c1 };

    u32 hash{ seed };

    hash ^= ((k << r1) | (k >> (32U - r1))) * c2;
    hash = ((hash << r2) | (hash >> (32U - r2))) * m + n;
    hash ^= 4U;
    hash ^= (hash >> 16U);
    hash *= 0x85ebca6bU;
    hash ^= (hash >> 13U);
    hash *= 0xc2b2ae35U;
    hash ^= (hash >> 16U);

    return hash;
}

static constexpr Vec3f32 gradients[] = {
    {1.F,1.F,0.F},{-1.F,1.F,0.F},{1.F,-1.F,0.F},{-1.F,-1.F,0.F},
    {1.F,0.F,1.F},{-1.F,0.F,1.F},{1.F,0.F,-1.F},{-1.F,0.F,-1.F},
    {0.F,1.F,1.F},{0.F,-1.F,1.F},{0.F,1.F,-1.F},{0.F,-1.F,-1.F}
};

static constexpr Vec2f32 gradients_2d[] = {
    {-1.F, 0.F}, {-1.F, -1.F}, {0.F, -1.F}, {1.F, -1.F}, 
    { 1.F, 0.F}, { 1.F, 1.F}, {0.F, 1.F}, {-1.F, 1.F}
};

static f32 dotGradient3dPerlin(u32 seed, Vec3i32 index, Vec3f32 point) {
    auto random_index = murmur3(index[0], seed);
    random_index = murmur3(index[1], random_index);
    random_index = murmur3(index[2], random_index);
    random_index = murmur3(seed, random_index);

    const auto gradient = gradients[random_index % 12];

    const auto dist = Vec3f32::sub(
        point,
        { 
            static_cast<f32>(index[0]), 
            static_cast<f32>(index[1]),
            static_cast<f32>(index[2])
        }
    );

    return Vec3f32::dot(dist, gradient);
}

static f32 dotGradient2dPerlin(u32 seed, Vec2i32 index, Vec2f32 point) {
    auto random_index = murmur3(index[0], seed);
    random_index = murmur3(index[1], random_index);
    random_index = murmur3(seed, random_index);

    const auto gradient = gradients_2d[random_index % 8];

    const auto dist = Vec2f32::sub(
        point,
        { 
            static_cast<f32>(index[0]), 
            static_cast<f32>(index[1])
        }
    );

    return Vec2f32::dot(dist, gradient);
}

static f32 dotGradient3dSimplex(u32 seed, Vec3i32 index, Vec3f32 point) {
    auto random_index = murmur3(index[0], seed);
    random_index = murmur3(index[1], random_index);
    random_index = murmur3(index[2], random_index);
    random_index = murmur3(seed, random_index);

    const auto gradient = gradients[random_index % 12];

    return Vec3f32::dot(gradient, point);
}

static f32 smootherStep(f32 t) {
    return t*t*t*(t*(t*6-15)+10);
}

f32 PerlinNoiseFunc2D::invoke(Vec2f32 point, u32 grid_density) {
    const Vec2i32 p00 {
        static_cast<i32>(point[0]) / static_cast<i32>(grid_density),
        static_cast<i32>(point[1]) / static_cast<i32>(grid_density),
    };

    const auto p11 = Vec2i32::add(p00, {1});

    point = Vec2f32::divScalar(point, static_cast<f32>(grid_density));

    const auto interpolation_weights = Vec2f32::sub(
        point, 
        { 
            static_cast<f32>(p00[0]), 
            static_cast<f32>(p00[1]),
        }
    );
    const Vec2f32 smoothed_weights {
        smootherStep(interpolation_weights[0]),
        smootherStep(interpolation_weights[1]),                
    };

    auto n0 = dotGradient2dPerlin(this->seed, p00, point);
    auto n1 = dotGradient2dPerlin(this->seed, {p11[0], p00[1]}, point);
    auto ix0 = (1.F - smoothed_weights[0]) * n0 + smoothed_weights[0] * n1;

    n0 = dotGradient2dPerlin(this->seed, {p00[0], p11[1]}, point);
    n1 = dotGradient2dPerlin(this->seed, {p11[0], p11[1]}, point);
    auto ix1 = (1.F - smoothed_weights[0]) * n0 + smoothed_weights[0] * n1;

    return (1.F - smoothed_weights[1]) * ix0 + smoothed_weights[1] * ix1;
}

f32 PerlinNoiseFunc3D::invoke(Vec3f32 point, u32 grid_density) {
    const Vec3i32 p000 {
        static_cast<i32>(point[0]) / static_cast<i32>(grid_density),
        static_cast<i32>(point[1]) / static_cast<i32>(grid_density),
        static_cast<i32>(point[2]) / static_cast<i32>(grid_density),
    };

    const auto p111 = Vec3i32::add(p000, {1});

    point = Vec3f32::divScalar(point, static_cast<f32>(grid_density));

    const auto interpolation_weights = Vec3f32::sub(
        point, 
        { 
            static_cast<f32>(p000[0]), 
            static_cast<f32>(p000[1]),
            static_cast<f32>(p000[2])
        }
    );
    const Vec3f32 smoothed_weights {
        smootherStep(interpolation_weights[0]),
        smootherStep(interpolation_weights[1]),                
        smootherStep(interpolation_weights[2])                
    };

    auto n0 = dotGradient3dPerlin(this->seed, p000, point);
    auto n1 = dotGradient3dPerlin(this->seed, {p111[0], p000[1], p000[2]}, point);
    auto ix0 = (1.F - smoothed_weights[0]) * n0 + smoothed_weights[0] * n1;

    n0 = dotGradient3dPerlin(this->seed, {p000[0], p111[1], p000[2]}, point);
    n1 = dotGradient3dPerlin(this->seed, {p111[0], p111[1], p000[2]}, point);
    auto ix1 = (1.F - smoothed_weights[0]) * n0 + smoothed_weights[0] * n1;

    const auto iy0 = (1.F - smoothed_weights[1]) * ix0 + smoothed_weights[1] * ix1;
    
    n0 = dotGradient3dPerlin(this->seed, {p000[0], p000[1], p111[2]}, point);
    n1 = dotGradient3dPerlin(this->seed, {p111[0], p000[1], p111[2]}, point);
    ix0 = (1.F - smoothed_weights[0]) * n0 + smoothed_weights[0] * n1;

    n0 = dotGradient3dPerlin(this->seed, {p000[0], p111[1], p111[2]}, point);
    n1 = dotGradient3dPerlin(this->seed, {p111[0], p111[1], p111[2]}, point);
    ix1 = (1.F - smoothed_weights[0]) * n0 + smoothed_weights[0] * n1;
    
    const auto iy1 = (1.F - smoothed_weights[1]) * ix0 + smoothed_weights[1] * ix1;

    return (1.F - smoothed_weights[2]) * iy0 + smoothed_weights[2] * iy1;
}

f32 SimplexNoiseFunc3D::invoke(Vec3f32 point, u32 grid_density) {
    // Based on implementation provided by Stefan Gustavson

    constexpr f32 skew_factor{ 1.F/3.F };
    constexpr f32 unskew_factor{ 1.F/6.F };

    const f32 s{ (point[0] + point[1] + point[2]) * skew_factor };

    const Vec3i32 ijk { 
        point[0]+s > 0.F ? static_cast<i32>(point[0] + s) : static_cast<i32>(point[0] + s) - 1,
        point[1]+s > 0.F ? static_cast<i32>(point[1] + s) : static_cast<i32>(point[1] + s) - 1,
        point[2]+s > 0.F ? static_cast<i32>(point[2] + s) : static_cast<i32>(point[2] + s) - 1
    };

    const f32 t{ 
        ((static_cast<f32>(ijk[0]) + static_cast<f32>(ijk[1]) + static_cast<f32>(ijk[2])) * unskew_factor)
    };

    const Vec3f32 p000 {
        static_cast<f32>(ijk[0]) - t,
        static_cast<f32>(ijk[1]) - t,
        static_cast<f32>(ijk[2]) - t
    };

    const auto dist = Vec3f32::sub(point, p000);

    Vec3i32 ijk_second_corner_offset(0);
    Vec3i32 ijk_third_corner_offset(0);

    if (dist[0] >= dist[1]) {
        if (dist[1] >= dist[2]) {
            ijk_second_corner_offset = Vec3i32(1, 0, 0);
            ijk_third_corner_offset = Vec3i32(1, 1, 0);
        } else if (dist[0] >= dist[2]) {
            ijk_second_corner_offset = Vec3i32(1, 0, 0);
            ijk_third_corner_offset = Vec3i32(1, 0, 1);
        } else {
            ijk_second_corner_offset = Vec3i32(0, 0, 1);
            ijk_third_corner_offset = Vec3i32(1, 0, 1);
        }
    } else {
        if (dist[1] < dist[2]) {
            ijk_second_corner_offset = Vec3i32(0, 0, 1);
            ijk_third_corner_offset = Vec3i32(0, 1, 1);
        } else if (dist[0] < dist[2]) {
            ijk_second_corner_offset = Vec3i32(0, 1, 0);
            ijk_third_corner_offset = Vec3i32(0, 1, 1);
        } else {
            ijk_second_corner_offset = Vec3i32(0, 1, 0);
            ijk_third_corner_offset = Vec3i32(1, 1, 0);
        }
    }

    const auto second_corner_offset = Vec3f32(
        (dist[0] - static_cast<f32>(ijk_second_corner_offset[0]) + unskew_factor),
        (dist[1] - static_cast<f32>(ijk_second_corner_offset[1]) + unskew_factor),
        (dist[2] - static_cast<f32>(ijk_second_corner_offset[2]) + unskew_factor)
    );
    const auto third_corner_offset = Vec3f32(
        (dist[0] - static_cast<f32>(ijk_third_corner_offset[0]) + 2.F * unskew_factor),
        (dist[1] - static_cast<f32>(ijk_third_corner_offset[1]) + 2.F * unskew_factor),
        (dist[2] - static_cast<f32>(ijk_third_corner_offset[2]) + 2.F * unskew_factor)
    );
    const auto last_corner_offset = Vec3f32::add(Vec3f32::sub(dist, Vec3f32(1.F)), Vec3f32(3.F * unskew_factor));

    const auto t0 = .5F - Vec3f32::dot(dist, dist);
    const auto n0 = (t0 < 0.F) ? 0.F : t0 * t0 * t0 * t0 *  dotGradient3dSimplex(this->seed, ijk, dist);

    const auto t1 = .5F - Vec3f32::dot(second_corner_offset, second_corner_offset);
    const auto n1 = (t1 < 0.F) ? 0.F : t1 * t1 * t1 * t1 *  dotGradient3dSimplex(this->seed, ijk, second_corner_offset);

    const auto t2 = .5F - Vec3f32::dot(third_corner_offset, third_corner_offset);
    const auto n2 = (t2 < 0.F) ? 0.F : t2 * t2 * t2 * t2 * dotGradient3dSimplex(this->seed, ijk, third_corner_offset);

    const auto t3 = .5F - Vec3f32::dot(last_corner_offset, last_corner_offset);
    const auto n3 = (t3 < 0.F) ? 0.F : t3 * t3 * t3 * t3 * dotGradient3dSimplex(this->seed, ijk, last_corner_offset);

    const auto result = 32.F * (n0 + n1 + n2 + n3);

    std::cout << result << std::endl;

    return result; //32.F * (n0 + n1 + n2 + n3);
}