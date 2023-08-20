#ifndef VE001_CUBEMAP_HPP
#define VE001_CUBEMAP_HPP

#include <vector>
#include <filesystem>
#include <array>
#include <variant>

#include <vmath/vmath_types.h>
#include "errors.h"

using namespace vmath;

namespace ve001 {

struct CubeMap {
    std::array<std::vector<u8>, 6> faces;
    u32 width{ 0U };
    u32 height{ 0U };
    u32 channels_num{ 0U };

    static Error load(CubeMap& self, const std::filesystem::path& img_path);
};

}

#endif