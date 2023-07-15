#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

using namespace ve001;

template<>
void Window::setKeyCallback
(void(*key_callback)(GLFWwindow*, i32, i32, i32, i32));

template<>
void Window::setMousePositionCallback
(void(*mouse_position_callback)(GLFWwindow*, f64, f64));