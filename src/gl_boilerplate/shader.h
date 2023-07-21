#ifndef VE001_SHADER_H
#define VE001_SHADER_H

#include <vmath/vmath_types.h>
#include <filesystem>

using namespace vmath;

namespace ve001 {

struct Shader {
    enum : std::size_t {
        VERTEX = 0, FRAGMENT = 1, COMPUTE = 0
    };

    u32 _prog_id{ 0U }; // shader program id
    u32 _shader_ids[2] = { 0U, 0U }; // compute shader id or vertex + fragment shader id

    /**
     * @brief compute shader constructor
     * @param csh_path compute shader path
    */
    Shader(const std::filesystem::path& csh_path);
    /**
     * @brief vertex-fragment shader constructor
     * @param vsh_path vertex shader path
     * @param fsh_path fragment shader path
    */
    Shader(const std::filesystem::path& vsh_path, const std::filesystem::path& fsh_path);

    /**
     * @brief binds shader program
    */
    void bind() const;

    /**
     * @brief deinitializes shader object
    */
    void deinit();
};

}

#endif