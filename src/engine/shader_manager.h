#ifndef VE001_SHADER_MANAGER_H
#define VE001_SHADER_MANAGER_H

#include <array>

#include "shader.h"

namespace ve001 {

enum class ShaderType : std::size_t {
    MULTI_LIGHTS_SHADER,
    SHADOW_MAP_DIR_SHADER,
    SHADOW_MAP_POINT_SHADER,
    SHADOW_MAP_SPOT_SHADER,
    COUNT 
};

struct ShaderManager {
    std::array<Shader, static_cast<std::size_t>(ShaderType::COUNT)> _shaders;

    void init();
    void deinit();

    auto& operator[](ShaderType shader_type) {
        return _shaders[static_cast<std::size_t>(shader_type)];
    }
};

}

#endif