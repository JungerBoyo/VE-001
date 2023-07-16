#include "vmath.h"

using namespace vmath;

template struct Vec<f32, 2>; 
template struct Vec<f32, 3>;
template struct Vec<f32, 4>;

template struct Vec<u32, 2>;
template struct Vec<u32, 3>;
template struct Vec<u32, 4>;

template struct Vec<i32, 2>;
template struct Vec<i32, 3>;
template struct Vec<i32, 4>;

template struct Mat<Vec<f32, 2>, 2>; 
template struct Mat<Vec<f32, 3>, 3>;
template struct Mat<Vec<f32, 4>, 4>;

template struct misc<f32>;
