#include "gl_context.h"

#include <stdexcept>

#include <glad/glad.h>

#if defined(VE001_USE_GLFW3)
    #define GLFW_INCLUDE_NONE
    #include <GLFW/glfw3.h>
#elif defined(VE001_USE_SDL2)
    #include <SDL2/SDL.h>
#endif

using namespace ve001;

void ve001::glInit() {
#if defined(VE001_USE_GLFW3)
	if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
		throw std::runtime_error("glad loader failed");
	}
#elif defined(VE001_USE_SDL2)
	if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)) == 0) {
		throw std::runtime_error("glad loader failed");
	}
#endif
}

void ve001::setGLDebugCallback(void (*gl_error_callback)(u32, u32, u32, u32, i32, const char *, const void *)) {
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageControl(
        GL_DONT_CARE, GL_DONT_CARE,
        GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
        GL_TRUE
    );
    glDebugMessageControl(
        GL_DONT_CARE, GL_DONT_CARE,
        GL_DEBUG_SEVERITY_LOW, 0, nullptr,
        GL_TRUE
    );
    glDebugMessageCallback(gl_error_callback, nullptr);
}