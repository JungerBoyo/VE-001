#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

template<>
void ve001::Window::setKeyCallback
(void(*key_callback)(GLFWwindow*, vmath::i32, vmath::i32, vmath::i32, vmath::i32));

template<>
void ve001::Window::setMousePositionCallback
(void(*mouse_position_callback)(GLFWwindow*, vmath::f64, vmath::f64));