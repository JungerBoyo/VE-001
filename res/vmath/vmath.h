#ifndef VE001_VMATH_H
#define VE001_VMATH_H

#include <numbers>
#include <cmath>
#include <type_traits>
#include <concepts>
#include <tuple>
#include "vmath_types.h"

namespace vmath {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

template<std::size_t S>
concept is_valid_vec_size = (S >= 2 && S <= 4);

template<typename T>
concept is_valid_vec_type = (std::is_floating_point_v<T> || std::is_integral_v<T>);

template<typename T, typename SameType>
concept is_valid_vec_type_or_same = (is_valid_vec_type<T> || std::is_same_v<T, SameType>);

template<typename T, std::size_t S> 
requires is_valid_vec_type<T> && is_valid_vec_size<S>
struct Vec {
    T values[S];

    inline constexpr T& operator[](std::size_t index) 
    {
        return values[index];
    }

    inline constexpr const T& operator[](std::size_t index) const
    {
        return values[index];
    }

    static constexpr std::size_t size = S;
    using ValueType = T;

    constexpr Vec() {
        for (std::size_t i{ 0U }; i < S; ++i) {
            values[i] = 0U;
        }
    }

    constexpr Vec(T value) {
        for (std::size_t i{ 0U }; i < S; ++i) {
            values[i] = value;
        }
    }

    template<typename ...Args>
    requires (sizeof...(Args) == S && (std::is_same_v<T, Args> && ...))
    constexpr Vec(Args... args) : values{ args... } {}

    static T dot(Vec<T, S> lhs, Vec<T, S> rhs) {
        T sum{ 0 };
        for (std::size_t i{ 0U }; i < S; ++i) {
            sum += lhs.values[i] * rhs.values[i];
        }
        return sum;
    }

    template<typename _T>
    requires is_valid_vec_type<_T>
    static Vec<T, S> cast(Vec<_T, S> vec) {
        Vec<T, S> result;
        for (std::size_t i{ 0U }; i < S; ++i) {
            result[i] = static_cast<T>(vec[i]);
        }
        return result;
    }



    T len() const {
        return std::sqrt(Vec<T, S>::dot(*this, *this));
    }

    static Vec<T, S> divScalar(Vec<T, S> vec, T scalar) {
        for (std::size_t i{ 0U }; i < S; ++i) {
            vec.values[i] /= scalar;
        }
        return vec;
    }

    static Vec<T, S> normalize(Vec<T, S> vec) {
        return Vec<T, S>::divScalar(vec, vec.len());
    }

    static Vec<T, S> negate(Vec<T, S> vec) {
        for (std::size_t i{ 0U }; i < S; ++i) {
            vec.values[i] = -vec.values[i];
        }
        return vec;
    }

    static Vec<T, S> sub(Vec<T, S> lhs, Vec<T, S> rhs) {
        for (std::size_t i{ 0U }; i < S; ++i) {
            lhs.values[i] -= rhs.values[i];
        }
        return lhs;
    }

    static Vec<T, S> add(Vec<T, S> lhs, Vec<T, S> rhs) {
        for (std::size_t i{ 0U }; i < S; ++i) {
            lhs.values[i] += rhs.values[i];
        }
        return lhs;
    }

    static Vec<T, S> mul(Vec<T, S> lhs, Vec<T, S> rhs) {
        for (std::size_t i{ 0U }; i < S; ++i) {
            lhs.values[i] *= rhs.values[i];
        }
        return lhs;
    }

    static Vec<T, S> mulScalar(Vec<T, S> vec, T scalar) {
        for (std::size_t i{ 0U }; i < S; ++i) {
            vec.values[i] *= scalar;
        }
        return vec;
    }
};


template<typename T>
Vec<T, 3> cross(Vec<T, 3> lhs, Vec<T, 3> rhs) {
    return Vec<T, 3>(
        lhs.values[1]*rhs.values[2] - rhs.values[1]*lhs.values[2], 
        lhs.values[2]*rhs.values[0] - rhs.values[2]*lhs.values[0],
        lhs.values[0]*rhs.values[1] - rhs.values[0]*lhs.values[1]
    );
}

template<std::size_t N, std::size_t M>
concept is_valid_mat_size = (N == M);

template<typename T> 
requires std::is_floating_point_v<T>
struct Quaternion;

template<typename V, std::size_t S>
requires is_valid_mat_size<V::size, S>
struct Mat {
    V values[S];

    inline constexpr V& operator[](std::size_t index) 
    {
        return values[index];
    }
    inline constexpr const V& operator[](std::size_t index) const
    {
        return values[index];
    }

    Mat() {
        for (std::size_t i{ 0U }; i < S; ++i) {
            values[i] = V();
        }
    }

    template<typename ...Args>
    Mat(Args... args) {
        static_assert(sizeof...(Args) == S || sizeof...(Args) == 1, "Incorrect number of arguments provided for Vec constructor or 1.");
        static_assert(((std::is_same_v<V, Args> || (is_valid_vec_type<Args> && sizeof...(Args) == 1)) && ...), "All arguments must have the same type as V or be of V::ValueType in case when sizeof...(Args) == 1.");

        if constexpr (sizeof...(Args) == 1) {
            const auto tmp = std::get<0>(std::forward_as_tuple(args...));
            for (std::size_t i{ 0 }; i < S; ++i) {
                values[i][i] = tmp;
            }
        } else {
            V tmp[sizeof...(Args)] = { args... };
            for (std::size_t i{ 0 }; i < S; ++i) {
                values[i] = tmp[i];
            }
        }
    }

    static V mulVec(Mat<V, S> lhs, V rhs) {
        V result(static_cast<typename V::ValueType>(0));
        for (std::size_t i{ 0 }; i < S; ++i) {
            result = V::add(result, V::mulScalar(lhs[i], rhs[i]));
        }
        return result;
    }
    
    static Mat<V, S> mulScalar(Mat<V, S> lhs, typename V::ValueType rhs) {
        for (std::size_t i{ 0 }; i < S; ++i) {
            lhs[i] = V::mulScalar(lhs[i], rhs);
        }
        return lhs;
    }

    static Mat<V, S> transpose(Mat<V, S> mat) {
        Mat<V, S> result{};
        for (std::size_t c{ 0 }; c < S; ++c) {
            for (std::size_t r{ 0 }; r < S; ++r) {
                result[c][r] = mat[r][c];
            }
        }
        return result;
    }

    static Mat<V, S> mul(Mat<V, S> lhs, Mat<V, S> rhs) {
        Mat<V, S> result{};
        for (std::size_t c{ 0 }; c < S; ++c) {
            for (std::size_t r{ 0 }; r < S; ++r) {
                result[c] = V::add(result[c], V::mulScalar(lhs[r], rhs[c][r]));
            }
        }
        return result;
    }

    typename V::ValueType det() const {
        if constexpr (S == 2) {
            return values[0][0] * values[1][1] - values[0][1]*values[1][0];
        } else if constexpr (S == 3) {
            return 
                values[0][0] * (values[1][1] * values[2][2] - values[2][1] * values[1][2]) -
                values[0][1] * (values[1][0] * values[2][2] - values[1][2] * values[2][0]) + 
                values[0][2] * (values[1][0] * values[2][1] - values[1][1] * values[2][0]);
        } else {
            const auto A2323 = values[2][2] * values[3][3] - values[2][3] * values[3][2];
            const auto A1323 = values[2][1] * values[3][3] - values[2][3] * values[3][1];
            const auto A1223 = values[2][1] * values[3][2] - values[2][2] * values[3][1];
            const auto A0323 = values[2][0] * values[3][3] - values[2][3] * values[3][0];
            const auto A0223 = values[2][0] * values[3][2] - values[2][2] * values[3][0];
            const auto A0123 = values[2][0] * values[3][1] - values[2][1] * values[3][0];
            return 
                values[0][0] * (values[1][1] * A2323 - values[1][2] * A1323 + values[1][3] * A1223) -
                values[0][1] * (values[1][0] * A2323 - values[1][2] * A0323 + values[1][3] * A0223) +
                values[0][2] * (values[1][0] * A1323 - values[1][1] * A0323 + values[1][3] * A0123) -
                values[0][3] * (values[1][0] * A1223 - values[1][1] * A0223 + values[1][2] * A0123);
        }
    }
};

template<typename T>
requires std::is_floating_point_v<T>
struct Quaternion {
    T w{ 0.0 };
    T i{ 0.0 };
    T j{ 0.0 };
    T k{ 0.0 };

    Quaternion(T w, T i, T j, T k) 
        : w(w), i(i), j(j), k(k) {}

    Quaternion(Vec<T, 3> rotation_axis, T angle) {
        const auto sin = std::sin(angle);
        *this = Quaternion<T>(
            std::cos(angle),
            sin * rotation_axis[0],
            sin * rotation_axis[1],
            sin * rotation_axis[2]
        );
    }

    Quaternion(Vec<T, 3> vec) 
        : w(0.0), i(vec[0]), j(vec[1]), k(vec[2]) {}

    static Quaternion<T> mul(Quaternion<T> lhs, Quaternion<T> rhs) {
        return Quaternion<T>(
            lhs.w * rhs.w - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
            lhs.w * rhs.i + lhs.i * rhs.w - lhs.k * rhs.j + lhs.j * rhs.k,
            lhs.w * rhs.j + lhs.j * rhs.w - lhs.i * rhs.k + lhs.k * rhs.i,
            lhs.w * rhs.k + lhs.k * rhs.w - lhs.j * rhs.i + lhs.i * rhs.j
        );
    }

    static Quaternion<T> negate(Quaternion<T> q) {
        q.i = -q.i;
        q.j = -q.j;
        q.k = -q.k;
        return q;
    }
};

template<typename T>
using Vec2 = Vec<T, 2>;
template<typename T>
using Vec3 = Vec<T, 3>;
template<typename T>
using Vec4 = Vec<T, 4>;

using Vec2f32 = Vec2<f32>;
using Vec3f32 = Vec3<f32>;
using Vec4f32 = Vec4<f32>;

using Vec2u32 = Vec2<u32>;
using Vec3u32 = Vec3<u32>;
using Vec4u32 = Vec4<u32>;

using Vec2i32 = Vec2<i32>;
using Vec3i32 = Vec3<i32>;
using Vec4i32 = Vec4<i32>;

template<typename T>
using Mat2 = Mat<Vec<T, 2>, 2>;
template<typename T>
using Mat3 = Mat<Vec<T, 3>, 3>;
template<typename T>
using Mat4 = Mat<Vec<T, 4>, 4>;

using Mat2f32 = Mat2<f32>;
using Mat3f32 = Mat3<f32>;
using Mat4f32 = Mat4<f32>;

template<typename T>
requires std::is_floating_point_v<T>
struct misc {
    static Mat4<T> symmetricPerspectiveProjection(T fov, T near, T far, T width, T height) {
        return Mat4<T>(
            Vec4<T>(static_cast<T>(1.0)/((width/height) * fov), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0)),
            Vec4<T>(static_cast<T>(0.0), static_cast<T>(1.0)/fov, static_cast<T>(0.0), static_cast<T>(0.0)),
            Vec4<T>(static_cast<T>(0.0), static_cast<T>(0.0), (-far - near)/(far - near), static_cast<T>(-1.0)),
            Vec4<T>(static_cast<T>(0.0), static_cast<T>(0.0), -(static_cast<T>(2.0) * far * near)/(far - near), static_cast<T>(0.0))
        );
    }

    static Mat4<T> rotationMatFromUnitQuaternion(Quaternion<T> q) {
        return Mat4<T>(
            Vec4<T>(
                static_cast<T>(1.0) - static_cast<T>(2.0)*(q.j*q.j + q.k*q.k), 
                static_cast<T>(2.0)*(q.i*q.j + q.w*q.k), 
                static_cast<T>(2.0)*(q.i*q.k - q.w*q.j), 
                static_cast<T>(0.0)
            ),
            Vec4<T>(
                static_cast<T>(2.0)*(q.i*q.j - q.w*q.k), 
                static_cast<T>(1.0) - static_cast<T>(2.0)*(q.i*q.i + q.k*q.k), 
                static_cast<T>(2.0)*(q.j*q.k + q.w*q.i), 
                static_cast<T>(0.0)
            ),
            Vec4<T>(
                static_cast<T>(2.0)*(q.i*q.k + q.w*q.j),
                static_cast<T>(2.0)*(q.j*q.k - q.w*q.i),
                static_cast<T>(1.0) - static_cast<T>(2.0)*(q.i*q.i + q.j*q.j),
                static_cast<T>(0.0)
            ),
            Vec4<T>(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0))
        );
    }

    static Mat4<T> rotationMatFromEulerAngles(Vec3<T> angles) {
        const auto rot_x = angles[0] == static_cast<T>(0.0) ? 
            Mat4<T>(1.0) :
            rotationMatFromUnitQuaternion(
                Quaternion<T>(Vec3<T>(static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0)), angles[0]/static_cast<T>(2.0)));
        const auto rot_y = angles[1] == static_cast<T>(0.0) ? 
            Mat4<T>(static_cast<T>(1.0)) :
            rotationMatFromUnitQuaternion(
                Quaternion<T>(Vec3<T>(static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0)), angles[1]/static_cast<T>(2.0)));
        const auto rot_z = angles[2] == static_cast<T>(0.0) ? 
            Mat4<T>(static_cast<T>(1.0)) :
            rotationMatFromUnitQuaternion(
                Quaternion<T>(Vec3<T>(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0)), angles[2]/static_cast<T>(2.0)));

        return Mat4<T>::mul(rot_z, Mat4<T>::mul(rot_y, rot_x));
    }

    static Vec3<T> rotatePoint3D(Vec3<T> angles, Vec3<T> point, Vec3<T> origin, Mat3<T> basis) {
        using Q = Quaternion<T>;

        const auto q_point = Q(Vec3<T>::sub(point, origin));

        const auto qx = Q(Mat3<T>::mulVec(basis, Vec3<T>(static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0))), angles[0]/static_cast<T>(2.0));
        const auto qy = Q(Mat3<T>::mulVec(basis, Vec3<T>(static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0))), angles[1]/static_cast<T>(2.0));
        const auto qz = Q(Mat3<T>::mulVec(basis, Vec3<T>(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0))), angles[2]/static_cast<T>(2.0));

        const auto rotated_q_point =
            Q::mul(Q::mul(
                qy,
                Q::mul(Q::mul(
                    qy,
                    Q::mul(Q::mul(
                        qx, 
                        q_point
                    ), Q::negate(qx))
                ), Q::negate(qy))
            ), Q::negate(qz));
        
        return Vec3<T>(
            rotated_q_point.i + origin[0],
            rotated_q_point.j + origin[1],
            rotated_q_point.k + origin[2]
        );
    }

    static T wrap(T value, T abs_limit, T wrap_to) {
        return std::abs(value) > abs_limit ? wrap_to : value;
    }
};

}

#endif