#ifndef VE001_SHADER_H
#define VE001_SHADER_H

#include <vmath/vmath_types.h>
#include <filesystem>

namespace ve001 {

struct Shader {
    enum : std::size_t {
        VERTEX = 0, FRAGMENT = 1, COMPUTE = 0, GEOMETRY = 2
    };

    vmath::u32 _prog_id{ 0U }; // shader program id
    vmath::u32 _shader_ids[3] = { 0U, 0U, 0U }; // compute shader id or vertex + fragment shader id or vertex + geom + fragment

    Shader() = default;
    /**
     * @brief initialize program
    */
    void init();

    /**
     * @brief attach compute shader
     * @param csh_path compute shader path
    */
    bool attach(const std::filesystem::path& csh_path);

    /**
     * @brief attach vertex-fragment shader
     * @param vsh_path vertex shader path
     * @param fsh_path fragment shader path
    */
    bool attach(const std::filesystem::path& vsh_path, const std::filesystem::path& fsh_path);

    /**
     * @brief attach vertex-fragment shader
     * @param vsh_path vertex shader path
     * @param gsh_path vertex shader path
     * @param fsh_path fragment shader path
    */
    bool attach(
        const std::filesystem::path& vsh_path, 
        const std::filesystem::path& gsh_path, 
        const std::filesystem::path& fsh_path
    );

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