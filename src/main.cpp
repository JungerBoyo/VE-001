#include <glad/glad.h>

#include <gl_context.h>
#include <window/window.h>
#include <vmath/vmath.h>

using namespace ve001;
using namespace vmath;

int main() {
    Vec3f32 a(3.2F);
    Vec3f32 b(2.55F);

    [[maybe_unused]] const auto c = cross(a, b);

    if (!ve001::window.init("demo", 640, 480, nullptr)) {
        return 1;
    }

    ve001::glInit();
    // ve001::setGLDebugCallback([](u32, u32, u32 id, u32, int, const char *message, const void *) {
	// 	fmt::print("[GLAD]{}:{}", id, message);
	// });

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

    return 0;
}