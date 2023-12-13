#include "gl_context.h"

#include <stdexcept>

#include <glad/glad.h>
#include <logger/logger.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

using namespace ve001;

static void glDebugCallback(unsigned int, unsigned int, unsigned int id, unsigned int severity, int, const char* msg, const void*) {
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
	if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
		throw std::runtime_error("glad loader failed");
	}

#ifdef DEBUG
   glEnable(GL_DEBUG_OUTPUT);
   glDebugMessageCallback(glDebugCallback, nullptr);
#endif
}