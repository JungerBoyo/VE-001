#include "gl_context.h"

#include <stdexcept>

#include <glad/glad.h>
#include <logger/logger.h>

#if defined(VE001_USE_GLFW3)
    #define GLFW_INCLUDE_NONE
    #include <GLFW/glfw3.h>
#elif defined(VE001_USE_SDL2)
    #include <SDL2/SDL.h>
#endif

using namespace ve001;

// #ifdef DEBUG
// #endif

static void glDebugCallback(u32, u32, u32 id, u32 severity, i32, const char* msg, const void*) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        logger->info("[GL]({}):{}", id, msg);
    break;
    case GL_DEBUG_SEVERITY_LOW:
        logger->warn("[GL]({}):{}", id, msg);
    break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        logger->error("[GL]({}):{}", id, msg);
    break;
    case GL_DEBUG_SEVERITY_HIGH: 
        logger->critical("[GL]({}):{}", id, msg);
    break;
    }
}

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

// #ifdef DEBUG
   glEnable(GL_DEBUG_OUTPUT);
    // glDebugMessageControl(
    //     GL_DONT_CARE, GL_DONT_CARE,
    //     GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
    //     GL_TRUE
    // );
    // glDebugMessageControl(
    //     GL_DONT_CARE, GL_DONT_CARE,
    //     GL_DEBUG_SEVERITY_LOW, 0, nullptr,
    //     GL_TRUE
    // );
   glDebugMessageCallback(glDebugCallback, nullptr);
// #endif
}

void ve001::setGLDebugCallback(void (*gl_error_callback)(u32, u32, u32, u32, i32, const char *, const void *)) {
    glEnable(GL_DEBUG_OUTPUT);
    // glDebugMessageControl(
    //     GL_DONT_CARE, GL_DONT_CARE,
    //     GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
    //     GL_TRUE
    // );
    // glDebugMessageControl(
    //     GL_DONT_CARE, GL_DONT_CARE,
    //     GL_DEBUG_SEVERITY_LOW, 0, nullptr,
    //     GL_TRUE
    // );
    glDebugMessageCallback(gl_error_callback, nullptr);
}
