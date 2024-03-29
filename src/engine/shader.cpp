#include "shader.h"

#include <exception>
#include <fstream>
#include <vector>

#include <glad/glad.h>

using namespace ve001;
using namespace vmath;

/**
 * @brief parse spirv code into std::vector
 * @param path shader path
*/
static std::vector<char> parseAsSpirv(const std::filesystem::path &path) noexcept;
/**
 * @brief create shader from spirv
 * @param path shader path
 * @param shader_id shader id, returned by glCreateShader call
 * @return true - on success, false - on error
*/
static bool createShaderFromSpirv(const std::filesystem::path &path, u32 shader_id) noexcept;
/**
 * @brief compile shader from source
 * @param path shader path
 * @param shader_id shader id, returned by glCreateShader call
 * @return true - on success, false - on error
*/
static bool compileShader(const std::filesystem::path &path, u32 shader_id) noexcept;

void Shader::init() noexcept {
    _prog_id = glCreateProgram();
}

bool Shader::attach(const std::filesystem::path& csh_path, bool spirv) noexcept {
	_shader_ids[COMPUTE] = glCreateShader(GL_COMPUTE_SHADER);

    if (spirv) {
        if (!createShaderFromSpirv(csh_path, _shader_ids[COMPUTE])) {
            return false;
        }
    } else {
        if (!compileShader(csh_path, _shader_ids[COMPUTE])) {
            return false;
        }
    }
    glAttachShader(_prog_id, _shader_ids[COMPUTE]);

	glValidateProgram(_prog_id);
	glLinkProgram(_prog_id);
    
    return true;
}

bool Shader::attach(const std::filesystem::path& vsh_path, const std::filesystem::path& fsh_path, bool spirv) noexcept {
	_shader_ids[VERTEX] = glCreateShader(GL_VERTEX_SHADER);
	_shader_ids[FRAGMENT] = glCreateShader(GL_FRAGMENT_SHADER);

    const auto make_shader = spirv ? createShaderFromSpirv : compileShader;

    if (!make_shader(vsh_path, _shader_ids[VERTEX])) {
        return false;
    }
    if (!make_shader(fsh_path, _shader_ids[FRAGMENT])) {
        return false;
    }

    glAttachShader(_prog_id, _shader_ids[VERTEX]);
    glAttachShader(_prog_id, _shader_ids[FRAGMENT]);
	glValidateProgram(_prog_id);
	glLinkProgram(_prog_id);

    return true;
}

bool Shader::attach(
    const std::filesystem::path& vsh_path, 
    const std::filesystem::path& gsh_path, 
    const std::filesystem::path& fsh_path,
    bool spirv
) noexcept {
	_shader_ids[VERTEX] = glCreateShader(GL_VERTEX_SHADER);
	_shader_ids[FRAGMENT] = glCreateShader(GL_FRAGMENT_SHADER);
    _shader_ids[GEOMETRY] = glCreateShader(GL_GEOMETRY_SHADER);

    const auto make_shader = spirv ? createShaderFromSpirv : compileShader;

    if (!make_shader(vsh_path, _shader_ids[VERTEX])) {
        return false;
    }
    if (!make_shader(fsh_path, _shader_ids[FRAGMENT])) {
        return false;
    }
    if (!make_shader(gsh_path, _shader_ids[GEOMETRY])) {
        return false;
    }
    glAttachShader(_prog_id, _shader_ids[VERTEX]);
    glAttachShader(_prog_id, _shader_ids[FRAGMENT]);
    glAttachShader(_prog_id, _shader_ids[GEOMETRY]);

	glValidateProgram(_prog_id);
	glLinkProgram(_prog_id);

    return true;
}

void Shader::bind() const noexcept {
	glUseProgram(_prog_id);
}

void Shader::deinit() noexcept {
	for (std::size_t i{ 0U }; i < 3U; ++i) {
        if (_shader_ids[i] != 0 ) {
            glDetachShader(_prog_id, _shader_ids[i]);
            glDeleteShader(_shader_ids[i]);
            _shader_ids[i] = 0;
        }
	}
	glDeleteProgram(_prog_id);
	_prog_id = 0;
}

std::vector<char> parseAsSpirv(const std::filesystem::path &path) noexcept {
    try {
        std::ifstream stream(path.c_str(), std::ios::binary | std::ios::ate);

        if (!stream.good()) {
            return {};
        }

        const auto size = static_cast<std::size_t>(stream.tellg());
        std::vector<char> code(size);

        stream.seekg(0);
        stream.read(code.data(), static_cast<std::streamsize>(size));

        stream.close();

        return code;
    } catch(const std::exception&) {
        return {};
    }
}

bool createShaderFromSpirv(const std::filesystem::path &path, u32 shader_id) noexcept {
    const auto sh_binary = parseAsSpirv(path);
    if (sh_binary.empty()) {
        return false; 
    }
    glShaderBinary(
        1, static_cast<const GLuint *>(&shader_id),
        GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
        static_cast<const void *>(sh_binary.data()),
        static_cast<GLsizei>(sh_binary.size())
    );
    glSpecializeShaderARB(shader_id, "main", 0, nullptr, nullptr);

    return true;
}
bool compileShader(const std::filesystem::path &path, u32 shader_id) noexcept {
    try {
        std::ifstream stream(path.c_str());
        std::stringstream sstream;
        for (std::string line; std::getline(stream, line);) {
            sstream << line << '\n';
        }
        const auto shader_src = sstream.str();
        
        if (shader_src.empty()) {
            return false;
        }

        const char* shader_src_cstr = shader_src.c_str();
        const auto len = static_cast<i32>(shader_src.length());

        glShaderSource(shader_id, 1, &shader_src_cstr, &len);
        glCompileShader(shader_id);
    } catch (const std::exception&) {
        return false;
    }
    return true;
}