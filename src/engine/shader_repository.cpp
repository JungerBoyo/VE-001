#include "shader_repository.h"
#include <glad/glad.h>
#include <vmath/vmath_types.h>

using namespace vmath;
using namespace ve001;

static constexpr std::string_view BASE_SHADERS_BIN = "shaders/bin/";
static constexpr std::string_view BASE_SHADERS_SRC = "shaders/src/";

struct ShaderDescriptor {
    enum class Type {
        VERT_FRAG,
        VERT_GEOM_FRAG,
        COMPUTE,
    };

    std::string_view name;
    Type type;
};

static constexpr std::array<ShaderDescriptor, static_cast<std::size_t>(ShaderType::COUNT)> SHADERS {{
    { "multi_lights_shader/",       ShaderDescriptor::Type::VERT_FRAG },
    { "shadow_map_dir_shader/",     ShaderDescriptor::Type::VERT_FRAG },
    { "shadow_map_point_shader/",   ShaderDescriptor::Type::VERT_GEOM_FRAG },
    { "shadow_map_spot_shader/",    ShaderDescriptor::Type::VERT_FRAG },
    { "greedy_meshing_shader/",     ShaderDescriptor::Type::COMPUTE }
}};

void ShaderRepository::init() {
    int ext_num{ 0 };
    bool is_arb_spirv_supported{ false };
    glGetIntegerv(GL_NUM_EXTENSIONS, &ext_num);
    for (int i{ 0 }; i < ext_num; ++i) {
        const auto ext_str = std::string_view(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)));
        if (ext_str == "GL_ARB_gl_spirv") {
            is_arb_spirv_supported = true;
            break;
        }
    }
    const auto BASE_SHADERS = is_arb_spirv_supported ? BASE_SHADERS_BIN : BASE_SHADERS_SRC;
    if (is_arb_spirv_supported) {
        i32 i{ 0 };
        const auto base_path = std::filesystem::path(BASE_SHADERS_BIN);
        for (const auto shader_desc : SHADERS) {
            _shaders[i].init();
            const auto shader_path = base_path / shader_desc.name;
            switch (shader_desc.type) {
                case ShaderDescriptor::Type::COMPUTE: {
                    // const auto base_path2 = std::filesystem::path(BASE_SHADERS_SRC);
                    // const auto shader_path2 = base_path2 / shader_desc.name;
                    _shaders[i].attach(shader_path / "comp.spv", true);
                    // _shaders[i].attach(shader_path2 / "shader.comp", false);
                    break;
                }
                case ShaderDescriptor::Type::VERT_FRAG: {
                    _shaders[i].attach(shader_path / "vert.spv", shader_path / "frag.spv", true);
                    break;
                }
                case ShaderDescriptor::Type::VERT_GEOM_FRAG: {
                    _shaders[i].attach(shader_path / "vert.spv", shader_path / "geom.spv", shader_path / "frag.spv", true);
                    break; 
                }
            }
            ++i;
        }
    } else {
        i32 i{ 0 };
        const auto base_path = std::filesystem::path(BASE_SHADERS_SRC);
        for (const auto shader_desc : SHADERS) {
            _shaders[i].init();
            const auto shader_path = base_path / shader_desc.name;
            switch (shader_desc.type) {
                case ShaderDescriptor::Type::COMPUTE: {
                    _shaders[i].attach(shader_path / "shader.comp", false);
                    break;
                }
                case ShaderDescriptor::Type::VERT_FRAG: {
                    _shaders[i].attach(shader_path / "shader.vert", shader_path / "shader.frag", false);
                    break;
                }
                case ShaderDescriptor::Type::VERT_GEOM_FRAG: {
                    _shaders[i].attach(shader_path / "shader.vert", shader_path / "shader.geom", shader_path / "shader.frag", false);
                    break; 
                }
            }
            ++i;
        }
    }
}
void ShaderRepository::deinit() {
    for (auto& shader : _shaders) {
        shader.deinit();
    }
}