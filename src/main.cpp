#include <fmt/core.h>
#include <glad/glad.h>

#include <gl_context.h>
#include <window/window.h>

using namespace ve001;

int main() {
    fmt::print("Hello {}!\n", "Woorld");

    ve001::Window window("demo", 640, 480, nullptr);

    ve001::glInit();
    ve001::setGLDebugCallback([](u32, u32, u32 id, u32, int, const char *message, const void *) {
		fmt::print("[GLAD]{}:{}", id, message);
	});

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

    while(!window.shouldClose()) {
        const auto [window_width, window_height] = window.size();
        glViewport(0, 0, window_width, window_height);

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        window.swapBuffers();
        window.pollEvents();
    }
    window.deinit();
}